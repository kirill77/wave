#pragma once

#include "vector.h"
#include "MyMisc.h"

template <class T, NvU32 N>
struct Box
{
	inline Box() { }
	inline Box(const rtvector<T, N>& v)
	{
		m_v[0] = v; m_v[1] = v;
	}
	inline Box(const rtvector<T, N>& v1, const rtvector<T, N>& v2)
	{
		m_v[0] = v1; m_v[1] = v2;
	}

	inline const rtvector<T, N>& operator[](NvU32 u) const { nvAssert(u < 2); return m_v[u]; }
	inline rtvector<T, N>& operator[](NvU32 u) { nvAssert(u < 2); return m_v[u]; }

private:
	rtvector<T, N> m_v[2];
};

typedef Box<float, 3> Box3f;