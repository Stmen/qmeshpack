#pragma once
#define SMALL_NUMBER  0.000001
#include <cmath>
#include <cassert>

/// Computes a vector of maximums of the components of @param v1 and @param v2
template <class T>
inline T max(T v1, T v2)
{
	v1 > v2 ? v1 : v2;
}

template <class T>
inline T min(T v1, T v2)
{
	v1 < v2 ? v1 : v2;
}


typedef float vec2f __attribute__ ((vector_size (8)));

typedef short vec2s16 __attribute__ ((vector_size (4)));
typedef unsigned short vec2u16 __attribute__ ((vector_size (4)));
typedef int vec2i __attribute__ ((vector_size (8)));
typedef unsigned int vec2u __attribute__ ((vector_size (8)));


/*
template <class T>
struct vec2
{
	typedef T vecdef_t __attribute__ ((vector_size (sizeof(T) * 2)));

};
*/
