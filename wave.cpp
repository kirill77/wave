#include "wave.h"

static inline void copyDim(Box3f& dstBox, const Box3f& srcBox, NvU32 uDim)
{
	dstBox[0][uDim] = srcBox[0][uDim];
	dstBox[1][uDim] = srcBox[1][uDim];
}
static inline void combineDim(Box3f& dstBox, const Box3f& smallBox, const Box3f& box, NvU32 uDim)
{
	dstBox[0][uDim] = smallBox[1][uDim];
	dstBox[1][uDim] = box[1][uDim];
}

void GridElem::split(const World &world, Storage &storage)
{
	Box3f box = world.computeElemBox(*this);
	Box3f smallBox(box[0], (box[0] + box[1]) / 2.f), tmpBox;

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

NvU32 Storage::allocateRoot(const float2& timePhase, const Box3f& box)
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

void Storage::visitInternal(GridElem* pElem, const Box3f& box, IVisitor& visitor)
{
	if (!visitor.notifyEntering(*pElem, box))
		return;

	if (pElem->hasChildren())
	{
		Box3f childBox;
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
void World::initialize()
{
	m_storage = Storage();
	NvU32 rootIndex = m_storage.allocateRoot(makefloat2(-1.f, 1.f), Box3f(makefloat3(-1.f), makefloat3(1.f)));
	auto& root = m_storage.accessRoot(rootIndex);
	// split 4 times to get some space differentiation going
	splitRecursive(root, 3);
}
Box3f World::computeElemBox(const GridElem& elem) const
{
	// find the root of the elem and its box
	NvU32 rootIndex = elem.computeRootIndex(m_storage);
	Box3f box = m_storage.getRootBox(rootIndex);

	// descend towards elem shrinking the box all the way
	for (auto *pElem = &m_storage.accessRoot(rootIndex); ; )
	{
		if (!pElem->hasChildren())
		{
			return box;
		}
		NvU32 childIndex = pElem->getFirstChild();
		auto boxCenter = (box[0] + box[1]) / 2.f;
		for (NvU32 uDim = 0; uDim < 3; ++uDim)
		{
			if (elem.getCenter()[uDim] >= boxCenter[uDim])
			{
				childIndex += (1 << uDim);
				box[0][uDim] = boxCenter[uDim];
			}
			else
			{
				box[1][uDim] = boxCenter[uDim];
			}
		}
		pElem = &m_storage[childIndex];
	}
}
void World::readPoints(std::vector<float3>& points)
{
	struct CollectPoints : public Storage::IVisitor
	{
		CollectPoints(std::vector<float3>& points) : m_points(points) { }
		virtual bool notifyEntering(GridElem& elem, const Box3f& box)
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
void World::splitRecursive(GridElem& elem, NvU32 depth)
{
	elem.split(*this, m_storage);
	if (depth == 0)
		return;
	--depth;
	auto* pChildren = &m_storage[elem.getFirstChild()];
	for (NvU32 u = 0; u < 8; ++u)
	{
		nvAssert(!pChildren[u].hasChildren());
		splitRecursive(pChildren[u], depth);
	}
}