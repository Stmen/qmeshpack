#pragma once
#define VECLIB_SOFT
#ifdef VECLIB_SOFT
#include "soft-vec.h"
#include "soft-quat.h"
#include "soft-mat.h"
#elif defined(VECLIB_SSE)
#include "sse-vec.h"
#include "sse-quat.h"
#include "sse-mat.h"
#elif defined(VECLIB_NEON)
#include "neon-vec.h"
#else
#error "define your configuration"
#endif

#include <type_traits>
static_assert(std::is_pod<vec3f>::value and std::is_trivial<vec3f>::value, "vec3f is not plain-old-data");
static_assert(std::is_pod<vec4f>::value and std::is_trivial<vec4f>::value, "vec4f is not plain-old-data");
static_assert(std::is_pod<quatf>::value and std::is_trivial<quatf>::value, "quatf is not plain-old-data");

///////////////////////////////////////////////////////////////////////////////
//
//    Returns the Projection of p on q
//
///////////////////////////////////////////////////////////////////////////////
template<class T>
inline T proj(const T p, const T q)
{
	return	dot(p, q) / dot(q, q) * q;
}

///////////////////////////////////////////////////////////////////////////////
//
//    Returns the perpendicular component of p on q
//
///////////////////////////////////////////////////////////////////////////////
template<class T>
T perp(const T p, const T q)
{
	return	p - proj(p, q);
}

