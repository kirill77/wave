#pragma once

template <class T, NvU32 N>
struct Storage
{
	NvU32 allocateNElems();
	void freeNElems(NvU32 u);
	inline NvU32 evalElemIndex(const T& el) const { return &el - &m_elems[0]; }
	inline T &getElem(NvU32 u) { return m_elems[u]; }

private:
	std::vector<T> m_elems;
	std::vector<NvU32> m_freeElems;
};

struct TimeBox
{
	// position aplitude. i don't really know what i am doing, so i am not sure if i also need velocity amplitude. trying with position for now
	double2 m_posAmpSum = double2(0);
	double m_weightsSum = 0;
};

// unlike space grid, the time grid is uniform. i believe electrons in bound state are supposed to be standing waves, so the amplitude in time
// is going to just rotate with some period. that's the reason we shouldn't need adaptive grid in time dimension
template <NvU32 N>
struct TimeGrid
{
	TimeBox& operator[](NvU32 u) { nvAssert(u < N); return m_timeBoxes[u]; }
	const TimeBox& operator[](NvU32 u) const { nvAssert(u < N); return m_timeBoxes[u]; }
private:
	TimeBox m_timeBoxes[N];
};

struct SpaceBox
{
	static void split(Storage<SpaceBox, 8> &tree, NvU32 newParentIndex)
	{
		NvU32 children = tree.allocateNElems();
		auto& parent = tree.getElem(newParentIndex);
		parent.m_children = children;
		for (NvU32 u = 0; u < 8; ++u)
		{
			auto& child = tree.getElem(children + u);
			child.m_parent = newParentIndex;
			child.m_pTimeBoxes = parent.m_pTimeBoxes;
			child.m_timeRoot = cloneTimeElems(parent.m_timeRoot, 1);
		}
	}

private:
	NvU32 cloneTimeElems(NvU32 uStart, NvU32 nElems)
	{
		NvU32 newStart = m_pTimeBoxes->allocateNElems();
		for (NvU32 u = 0; u < nElems; ++u)
		{
			if (newTimeElm.m_children != ~0)
			{
				newTimeElem.m_children = cloneTimeElems(newTimeElem.m_children, 2);
			}
		}
	}
	std::shared_ptr<TimeGrid> m_pTimeGrid;
	NvU32 m_children = ~0, m_parent = ~0;
};

