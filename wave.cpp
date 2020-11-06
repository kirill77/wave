#include "wave.h"
#include "Power2Distribution.h"

static double s_fQConst = 1;
static double s_fMConst = 1;

void generatePathTime(const float3 &fromP, const float3 &toP, double f01Number, double &fPathAction, double &fPathTime, double &fPathWeight)
{
	float3 d = toP - fromP;
	// in general case action at each point p is computed as:
	// fAction(p) = fKineticConstant * sqr(fSpeed(p)) - fPotential(p);
	//
	// for simplicity we assume that speed is constant between two points. potential is not
	// though because then force would be zero (force is derivative of potential), so:
	// fAction(p) = fKineticConstant * sqr(fSpeed) - fPotential(p)
	//
	// if T is the time it takes for electron to fly between the two points, then path action
	// would be equal to:
	// fPathAction(T) = ntgrl_t_0_T(fKineticConstant * sqr(fSpeed) - fPotential(p))
	//
	// let's introduce those variables:
	double dd = dot(d, d);
	double dp = dot(d, fromP);
	double pp = dot(fromP, fromP);
	// We use matlab to compute all of the above integrals symbolically:
	// syms px py pz dx dy dz t T
	// rx = px + dx * (t/T)
	// ry = py + dy * (t/T)
	// rz = pz + dz * (t/T)
	// collect(expand(rx^2+ry^2+rz^2),t) = (dd / T^2) * t^2 + (2 * dp / T) * t + pp
	//
	// syms t dd dp pp fQConst T fMConst
	// rr = (dd / T^2) * t^2 + (2 * dp / T) * t + pp
	//
	// potential energy: V = fQConst / sqrt(rr)
	// intV = int(V,t)
	// pathV = subs(intV,t,T) - subs(intV,t,0)
	// pathV = T * fQConst * log((dd + dp + sqrt(dd * (dd + 2 * dp + pp)))/(dp + sqrt(dd*pp)))/sqrt(dd)
	// Few things to notice about pathV:
	// * it is proportional to T, so it changes from 0 to inf as T grows
	// * it must be > 0 because we're integrating positive function: fQConst / sqrt(rr)
	//
	// kinetic energy: P = fMConst * dd / T^2
	// intP = int(P,t)
	// pathP = subs(intP,t,T) - subs(intP,t,0)
	// pathP = dd * fMConst / T
	// Few things to note about pathP:
	// * it is proportional to 1 / T, so it changes from inf to 0 as T grows
	// * it must be > 0
	//
	// pathA = pathP - pathV
	double Thelper = s_fQConst * log((dd + dp + sqrt(dd * (dd + 2 * dp + pp))) / (dp + sqrt(dd * pp)));
	nvAssert(Thelper >= 0);
	// pathA = (dd * fMConst)/T - T * Thelper/ dd^(1/2)
	// syms dd fMConst T Thelper
	// due to properties of pathP and pathV, there must be a point T0 where pathA = 0
	// we find that point T0 and then we generate random T sample around that point. that
	// sample will represent path between two points
	//
	// pathAeq = pathA == 0
	double fT0 = sqrt(Thelper * dd * sqrt(dd) * s_fMConst) / Thelper;

	double fTMin = fT0 / 2, fTMax = fT0 * 2;
	fPathWeight = 1;
	while (f01Number >= 0.5)
	{
		f01Number = (f01Number - 0.5) * 2;
		fPathWeight *= 2;
	}
}

static inline void copyDim(float3Box& dstBox, const float3Box& srcBox, NvU32 uDim)
{
	dstBox[0][uDim] = srcBox[0][uDim];
	dstBox[1][uDim] = srcBox[1][uDim];
}
static inline void combineDim(float3Box& dstBox, const float3Box& smallBox, const float3Box& box, NvU32 uDim)
{
	dstBox[0][uDim] = smallBox[1][uDim];
	dstBox[1][uDim] = box[1][uDim];
}

void GridElem::split(const World &world, Storage &storage, const float3Box &_box)
{
	float3Box box = _box;
	float3Box smallBox(box[0], (box[0] + box[1]) / 2.f), tmpBox;

	m_firstChildIndex = storage.allocate8Children();

	auto boxCenter = (box[0] + box[1]) / 2.f;
	copyDim(tmpBox, smallBox, 2);
	for (NvU32 z = 0, childIndex = m_firstChildIndex, myIndex = isRoot() ? storage.getRootIndex(*this) : storage.getChildIndex(*this); z < 2; ++z)
	{
		copyDim(tmpBox, smallBox, 1);
		for (NvU32 y = 0; y < 2; ++y)
		{
			copyDim(tmpBox, smallBox, 0);
			for (NvU32 x = 0; x < 2; ++x)
			{
				storage[childIndex++].initAsChild(m_timePhase, (tmpBox[0] + tmpBox[1]) / 2.f, isRoot(), myIndex);
				combineDim(tmpBox, smallBox, box, 0);
			}
			combineDim(tmpBox, smallBox, box, 1);
		}
		combineDim(tmpBox, smallBox, box, 2);
	}
}

NvU32 GridElem::computeRootIndex(const Storage& storage) const
{
	return isRoot() ? storage.getRootIndex(*this) :
		(isChildOfRoot() ? m_parentIndex : storage[m_parentIndex].computeRootIndex(storage));
}

NvU32 Storage::allocateRoot(const float2& timePhase, const float3Box& box)
{
	NvU32 rootIndex = (NvU32)m_pRoots.size();
	m_pRoots.resize(rootIndex + 1);
	m_pRootBoxes.push_back(box);
	m_pRoots[rootIndex].initAsRoot(timePhase, (box[0] + box[1]) / 2.f);
	return rootIndex;
}

NvU32 Storage::allocate8Children()
{
	if (m_firstFreeChild >= m_pChildren.size())
	{
		m_firstFreeChild = m_pChildren.size();
		m_pChildren.resize(m_firstFreeChild + 64);
		for (NvU32 u = m_firstFreeChild; u < m_pChildren.size(); ++u)
		{
			m_pChildren[u].setFirstChild(u + 8);
		}
	}
	NvU32 firstElemIndex = m_firstFreeChild;
	auto *pFirstElem = &m_pChildren[m_firstFreeChild];
	m_firstFreeChild = pFirstElem->getFirstChild();
	// clear all returned elements
	for (NvU32 u = 0; u < 8; ++u)
	{
		pFirstElem[u] = GridElem();
	}
	return firstElemIndex;
}

void Storage::visitInternal(GridElem* pElem, const float3Box& box, IVisitor& visitor)
{
	if (!visitor.notifyEntering(*pElem, box))
		return;

	if (pElem->hasChildren())
	{
		float3Box childBox;
		childBox[0] = box[0];
		childBox[1] = (box[0] + box[1]) / 2.f;
		NvU32 firstChildIndex = pElem->getFirstChild();
		for (NvU32 uChild = 0; ; )
		{
			visitInternal(&m_pChildren[firstChildIndex + uChild], childBox, visitor);
			NvU32 uDim = (uChild & 1) + ((uChild == 3) ? 1 : 0);
			if (++uChild == 8)
				break;
			// coords at uDim increase, everything below decreases
			childBox[0][uDim] = childBox[1][uDim];
			childBox[1][uDim] = box[1][uDim];
			while (--uDim < 3)
			{
				childBox[1][uDim] = childBox[0][uDim];
				childBox[0][uDim] = box[0][uDim];
			}
		}
	}

	visitor.notifyLeaving(*pElem, box);
}

struct SplitVisitor : public Storage::IVisitor
{
	SplitVisitor(World &world, Storage &storage, NvU32 depth) : m_world(world), m_storage(storage), m_depth(depth) { }

	virtual bool notifyEntering(GridElem& elem, const float3Box& box)
	{
		if (m_depth == 0)
			return false;
		elem.split(m_world, m_storage, box);
		--m_depth;
		return true;
	}
	virtual void notifyLeaving(GridElem& elem, const float3Box& box)
	{
		++m_depth;
	}

private:
	NvU32 m_depth;
	World& m_world;
	Storage& m_storage;
};

void World::initialize()
{
	m_storage = Storage();
	NvU32 rootIndex = m_storage.allocateRoot(makefloat2(-1.f, 1.f), float3Box(makefloat3(-1.f), makefloat3(1.f)));

	SplitVisitor splitVisitor(*this, m_storage, 3);
	m_storage.visit(0, splitVisitor);
}

void World::readPoints(std::vector<float3>& points)
{
	struct CollectPoints : public Storage::IVisitor
	{
		CollectPoints(std::vector<float3>& points) : m_points(points) { }
		virtual bool notifyEntering(GridElem& elem, const float3Box& box)
		{
			if (!elem.hasChildren())
			{
				for (NvU32 uDim = 0; uDim < 3; ++uDim)
				{
					m_points.push_back(box[0]);
					float3 otherPoint = box[0];
					otherPoint[uDim] = box[1][uDim];
					m_points.push_back(otherPoint);
				}
			}
			return true;
		}
		std::vector<float3>& m_points;
	};
	CollectPoints visitor(points);
	m_storage.visit(0, visitor);
}

void World::makeSimulationStep()
{
	struct SimVisitor2 : public Storage::IVisitor
	{
		SimVisitor2(GridElem& elemOfInterest, const float3Box& boxOfInterest) : m_elemOfInterest(elemOfInterest), m_boxOfInterest(boxOfInterest) { }
		virtual bool notifyEntering(GridElem& elem, const float3Box& box)
		{
			if (!doTouch(box, m_boxOfInterest))
			{
				return false;
			}
			if (elem.hasChildren())
			{
				return true;
			}
			if (&elem == &m_elemOfInterest)
			{
				return false;
			}
			// collect influence from elem to m_elemOfInterest
			return false;
		}
        private:
			GridElem& m_elemOfInterest;
            const float3Box& m_boxOfInterest;
	};
	struct SimVisitor1 : public Storage::IVisitor
	{
		SimVisitor1(Storage& storage) : m_storage(storage) { }
		virtual bool notifyEntering(GridElem& elem, const float3Box& box)
		{
			if (elem.hasChildren())
			{
				return true;
			}
			// find all the neighbors of that elem
			SimVisitor2 visitor(elem, box);
			m_storage.visit(0, visitor);
			return false;
		}
	private:
		Storage& m_storage;
	};
	SimVisitor1 visitor(m_storage);
	m_storage.visit(0, visitor);
}