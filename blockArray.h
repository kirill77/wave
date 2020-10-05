#pragma once

#include <vector>
#include <memory>

template <class T, NvU32 BLOCK_SIZE=64>
struct BlockArray
{
	NvU32 size() const { return m_size; }
	void resize(NvU32 size)
	{
		m_pBlocks.resize(NV_ALIGN_UP(size, BLOCK_SIZE) / BLOCK_SIZE);
		m_size = size;
	}
	inline T& operator[](NvU32 index) { return m_pBlocks[index / BLOCK_SIZE].p->data[index % 64]; }
	inline const T& operator[](NvU32 index) const { return m_pBlocks[index / BLOCK_SIZE].p->data[index % 64]; }

private:
	NvU32 m_size = 0;
	struct Block
	{
		T data[64];
	};
	struct BlockPtr
	{
		BlockPtr() { p = std::make_shared<Block>(); }
		std::shared_ptr<Block> p;
	};
	std::vector<BlockPtr> m_pBlocks;
};