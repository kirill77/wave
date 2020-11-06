#pragma once

#include "vector.h"

// Macro to define conversion and subscript operators
#define BOX_MEMBERS(T, n) \
			rtbox<T,n>(const rtvector<T,n> &vmin, const rtvector<T,n> &vmax) \
			{ (*this)[0] = vmin; (*this)[1] = vmax; } \
            rtbox<T,n>() { } \
			/* Subscript operators - built-in subscripts are ambiguous without these */ \
			rtvector<T,n> & operator [] (int i) \
			{ nvAssert(i < 2); return m_data[i]; } \
			const rtvector<T,n> & operator [] (int i) const \
			{ nvAssert(i < 2); return m_data[i]; } \
			/* Conversion to bool is not allowed (otherwise would happen implicitly through array conversions) */ \
			private: operator bool();

template <typename T, int n>
struct rtbox
{
    rtvector<T, n> m_data[2];
    BOX_MEMBERS(T, n)
};

#undef BOX_MEMBERS

#define DEFINE_CONCRETE_BOXES(type) \
			typedef rtbox<type, 2> type##2##Box; \
			typedef rtbox<type, 3> type##3##Box;

DEFINE_CONCRETE_BOXES(float);
DEFINE_CONCRETE_BOXES(double);

#undef DEFINE_CONCRETE_BOXES

template <class BOX1, class BOX2>
bool doTouch(const BOX1& box1, const BOX2& box2)
{
    return !any(box1[1] < box2[0]) && !any(box2[1] < box1[0]);
}