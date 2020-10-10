#pragma once

#include "box.h"
#include "blockArray.h"

struct Storage;
struct World;

struct GridElem
{
	inline GridElem() : m_isChildOfRoot(0), m_parentIndex(INVALID_PARENT_INDEX) { }

	void split(const World& world, Storage& storage, const Box3f &box);

	bool isRoot() const { return m_parentIndex == INVALID_PARENT_INDEX; }
	bool hasChildren() const { return m_firstChildIndex != INVALID_CHILD_INDEX; }
	NvU32 getFirstChild() const { nvAssert(hasChildren()); return m_firstChildIndex; }
	void setFirstChild(NvU32 index) { m_firstChildIndex = index; }
	bool isChildOfRoot() const { return m_isChildOfRoot; }
	NvU32 getParentIndex() const { return m_parentIndex; }
	NvU32 computeRootIndex(const Storage& storage) const;
	const float3& getCenter() const { return m_vCenter; }

	void initAsRoot(const float2& timePhase, const float3& vCenter)	{ m_timePhase = timePhase; m_vCenter = vCenter;	}

private:
	static const NvU32 INVALID_PARENT_INDEX = 0x7fffffffU;
	static const NvU32 INVALID_CHILD_INDEX = 0xffffffffU;
	void initAsChild(const float2& timePhase, const float3& vCenter, NvU32 isChildOfRoot, NvU32 parentIndex)
	{
		m_timePhase = timePhase; m_vCenter = vCenter; m_isChildOfRoot = isChildOfRoot, m_parentIndex = parentIndex;
	}
	float2 m_timePhase = makefloat2(1.f);
	float3 m_vCenter = makefloat3(123456.f);
	NvU32 m_firstChildIndex = INVALID_CHILD_INDEX;
	NvU32 m_isChildOfRoot : 1;
	NvU32 m_parentIndex : 31;
};

struct Storage
{
	NvU32 allocateRoot(const float2& timePhase, const Box3f& box);
	const GridElem& accessRoot(NvU32 u) const { return m_pRoots[u]; }
	GridElem& accessRoot(NvU32 u) { return m_pRoots[u]; }
	const Box3f& getRootBox(NvU32 u) const { return m_pRootBoxes[u]; }

	NvU32 allocate8Children();
	inline NvU32 getRootIndex(const GridElem& elem) const { return (NvU32)(&elem - &m_pRoots[0]); }
	inline NvU32 getChildIndex(const GridElem& elem) const
	{
		auto& parent = elem.isChildOfRoot() ? m_pRoots[elem.getParentIndex()] : m_pChildren[elem.getParentIndex()];
		NvU32 firstChildIndex = parent.getFirstChild();
		return firstChildIndex + (NvU32)(&elem - &m_pChildren[firstChildIndex]);
	}
	inline GridElem& operator[](NvU32 index) { return m_pChildren[index]; }
	inline const GridElem& operator[](NvU32 index) const { return m_pChildren[index]; }

	struct IVisitor
	{
		virtual bool notifyEntering(GridElem& elem, const Box3f& box) = 0;
		virtual void notifyLeaving(GridElem& elem, const Box3f& box) { }
	};
	inline void visit(NvU32 rootIndex, IVisitor& visitor)
	{
		visitInternal(&m_pRoots[rootIndex], m_pRootBoxes[rootIndex], visitor);
	}

private:
	void visitInternal(GridElem* pElem, const Box3f& box, IVisitor& visitor);
	std::vector<GridElem> m_pRoots; 
	std::vector<Box3f> m_pRootBoxes;
	BlockArray<GridElem> m_pChildren; // this is primary grid everyone is working with
	NvU32 m_firstFreeChild = ~0;
};

struct World
{
	void initialize();
	void readPoints(std::vector<float3>& points);
	void makeSimulationStep();

private:
	Storage m_storage;
};
