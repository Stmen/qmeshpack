#pragma once
#include <stddef.h>
#include <cstdlib>
#include <immintrin.h> // sse1
#include "common-vec.h"
#define ALIGNED16 __attribute__ ((aligned (16)))

#define VEC_X 0
#define VEC_Y 1
#define VEC_Z 2
#define VEC_W 3

#define SIMD_NEW_DELETE \
inline void* operator new(size_t sz) \
{ \
    void* mem = _mm_malloc(sz, 16); \
    if(!mem) abort(); \
    return mem; \
} \
inline void* operator new[] (size_t sz) \
{ \
    void* mem = _mm_malloc(sz, 16); \
    if(!mem) abort(); \
    return mem; \
} \
inline void operator delete(void* ptr) { _mm_free(ptr); } \
inline void operator delete[](void* ptr) { _mm_free(ptr); }


///////////////////////////////////////////////////////////////////////////////
//
//  Shuffles a vector
//
//  original _MM_SHUFFLE(z, y, x, w):  (z<<6) | (y<<4) | (x<<2) | w
//
///////////////////////////////////////////////////////////////////////////////
namespace sse
{
template<unsigned c>
inline __m128 fromComponent(const __m128 vec)
{
	static_assert((c < 4), "component index too big");
	return _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(c, c, c, c));
}

template<unsigned x, unsigned y, unsigned z, unsigned w>
inline __m128 shuffle(const __m128 vec)
{
	//return _mm_shuffle_ps(vec, vec, (((w) & 3 ) << 6 | ((z) & 3 ) << 4 | ((y) & 3 ) << 2 | ((x) & 3 )));
	return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(vec), (((w) & 3 ) << 6 | ((z) & 3 ) << 4 | ((y) & 3 ) << 2 | ((x) & 3 ))));
}

template<>
inline __m128 shuffle<0, 1, 2, 3>(const __m128 vec)
{
	return vec;
}

template<>
inline __m128 shuffle<2, 2, 3, 3>(const __m128 vec)
{
	return _mm_unpackhi_ps(vec, vec);
}

template<>
inline __m128 shuffle<2, 3, 2, 3>(const __m128 vec)
{
	return _mm_movehl_ps(vec, vec);
}

template<>
inline __m128 shuffle<0, 1, 0, 1>(const __m128 vec)
{
	return _mm_movelh_ps(vec, vec);
}

template<>
inline __m128 shuffle<0, 0, 1, 1>(const __m128 vec)
{
	return _mm_unpacklo_ps(vec, vec);
}

template<unsigned x, unsigned y, unsigned z, unsigned w>
inline __m128 shuffle(const __m128 v1, const __m128 v2)
{
	return _mm_shuffle_ps(v1, v2, (((w) & 3) << 6 | ((z) & 3) << 4 | ((y) & 3) << 2 | ((x) & 3)));
}


#define SHIFT_BYTES_LEFT(vec, b) (_mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(vec), b)))


#define SHIFT_BYTES_RIGHT(vec, b) (_mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(vec), b)))


/*
inline __m128 clear_w(const __m128 vec)
{
	return _mm_castsi128_ps(_mm_srli_si128(_mm_slli_si128(_mm_castps_si128((vec)), 4), 4));

}
*/


}; // namespace sse

#pragma pack(16)
struct vec4f;

struct vec3f
{
	__m128 vec;

	SIMD_NEW_DELETE;

	inline vec3f	operator+(const vec3f other) const { return { vec + other.vec }; }
	inline vec3f	operator-(const vec3f other) const { return { vec - other.vec }; }
	inline vec3f	operator/(const vec3f other) const { __m128 mask = {0.f, 0.f, 0.f, 1.f}; return { vec / (other.vec + mask) }; }
	inline vec3f	operator/(const float f) const { __m128 v = {f, f, f, f}; return { vec / v }; }
	inline vec3f	operator*(const vec3f other) const { return { vec * other.vec }; }
	inline vec3f	operator*(const float num) const { return { vec * from(num).vec }; }
	inline void	operator+=(const vec3f other) { vec += other.vec; }
	inline void	operator-=(const vec3f other) { vec -= other.vec; }
	inline void	operator/=(const vec3f other) { *this = *this / other; }
	inline void	operator*=(const vec3f other) { vec *= other.vec; }
	inline vec3f	operator<(const vec3f other) { vec3f v = { _mm_cmplt_ss(vec, other.vec) }; return v.mask<1, 1, 1>(); }
	inline vec3f	operator-()const { __v4si mask = {(int)(1 << 31), (int)(1 << 31), (int)(1 << 31), 0}; return { _mm_xor_ps(vec, (__m128)mask) }; }
	//inline float 	x()const { float x; _mm_store_ss(&x, vec); return x; }
	inline float 	x() const { return *(float*)&vec; }
	inline float 	y() const { return *((float*)&vec + 1); }
	inline float 	z() const { return *((float*)&vec + 2); }
	inline float 	elem(const unsigned i)const { return *((float*)&vec + i); }
	inline bool 	errorCheck()const { return elem(3) == 0.f; }

	template<unsigned mx = 0, unsigned my = 0, unsigned mz = 0>
	inline vec3f	mask() const
	{
		__v4si mask = { mx ? -1 : 0,
						my ? -1 : 0,
						mz ? -1 : 0,
						0 };
		return { _mm_and_ps(vec, (__m128)mask) };
	}

	/// returns true if these vector are equal.
	inline bool operator==(const vec3f other) const
	{
		int mask =_mm_movemask_ps(_mm_cmpeq_ps(vec, other.vec));
		return ((mask & 7) == 7);
	}

	inline bool operator!=(const vec3f other)const { return not (*this == other); }

	inline vec3f abs()const
	{
		__m128i tmp = _mm_castps_si128(vec);
		tmp = _mm_slli_epi32(tmp, 1);
		tmp = _mm_srli_epi32(tmp, 1);
		return { _mm_castsi128_ps(tmp) };
	}


	inline vec3f floor() const
	{
		#ifdef __SSE4__
		return _mm_floor_ps(vec);
		#else
		__m128 v1 = _mm_cvtepi32_ps(_mm_cvttps_epi32(vec)); // remove fractionals
		__m128 tmp = vec - v1; // store fractionals in vec

		// convert fractionals to 0 or 1
		tmp = _mm_cvtepi32_ps(_mm_srli_epi32(_mm_castps_si128(tmp), 31));
		v1 = v1 - tmp;
		return { v1 };
		#endif
	}

	inline vec3f ceil() const
	{
		#ifdef __SSE4__
		return { _mm_ceil_ps(vec) };
		#else
		__m128 tmp = _mm_cvtepi32_ps(_mm_cvttps_epi32(vec)); // remove fractionals
		tmp += _mm_cvtepi32_ps(_mm_srli_epi32(_mm_castps_si128(tmp - vec), 31));
		return { tmp };
		#endif
	}

	/// creates a permutation of this vector's components
	template<unsigned m0, unsigned m1, unsigned m2>
	inline vec3f shuffle() const
	{
		static_assert(m0 < 4 and m1 < 4 and m2 < 4, "bad suffle index");
		return { sse::shuffle<m0, m1, m2, 3>(vec) };
	}

	/// Adds all the elements of the vector and stores the result in each component
	inline vec3f hsum() const
	{
		return *this + shuffle<1, 2, 0>() + shuffle<2, 0, 1>();
	}

	/// horizontally adds all components and puts the result into x, other components are undefined
	inline vec3f hsum1() const
	{
		return hsum();
	}

	/// square root of x
	inline vec3f sqrt1() const
	{
		return { _mm_sqrt_ss(vec) };
	}

	/// square root in all components
	inline vec3f sqrt() const
	{
		return { _mm_sqrt_ps(vec) };
	}

	/// computes a squared length of this vector and puts it in each component of this vector
	inline float lengthSquared() const
	{
		return (*this * *this).hsum1().x();
	}

	/// computes a length of this vector and puts it in each component of this vector
	inline float length() const
	{
		vec3f t = { (vec * vec) };
		return t.hsum1().sqrt1().x();
	}

	/// computes normalized version of this vector.
	inline vec3f normalize() const
	{
		return *this / length();
	}

	template<unsigned char components>
	inline vec3f shiftComponentsRight() const
	{
		static_assert((components < 5), "bad component index");
		vec3f v = { SHIFT_BYTES_RIGHT(vec, components * 4) };
		return v.mask<1, 1, 1>();
	}

	template<unsigned char components>
	inline vec3f shiftComponentsLeft() const
	{
		static_assert((components < 5), "bad component index");
		return { SHIFT_BYTES_LEFT(vec, components * 4) };
	}

	/// Creates a vec3f from a vector @param v @param component
	template<unsigned c>
	inline vec3f fromComponent() const
	{
		static_assert((c < 3), "bad component index");
		return shuffle<c, c, c>();
	}

	/// create a vector from a floating point numbers.
	inline static vec3f from(const float x, const float y, const float z)
	{
		return { _mm_set_ps(0., z, y, x) };
	}

    /// create a vector from some values in ram.
    inline static vec3f from(float* ptr)
    {
        return vec3f::from(ptr[0], ptr[1], ptr[2])
    }

	/// create a v3f from a floating point number @param a
	inline static vec3f from(const float a)
	{
		__m128 tmp = _mm_load_ss(&a);
		return  { _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(3, 0, 0, 0)) };
	}

	/// Creates a vector @param x and zeroes the rest.
	inline static vec3f fromX00(const float x)
	{
		return { _mm_load_ss(&x) };
	}

	vec4f toVec4f() const;

	struct packed
	{
		float elems[3];
		vec3f toVec3f() { vec3f r = { { elems[0], elems[1], elems[2], 0 } }; return r;  }
	};

	struct packed pack()
	{
		return { { x(), y(), z() } };
	}
};

struct vec4f
{
	__m128 vec;

    SIMD_NEW_DELETE

	inline vec4f	operator+(const vec4f other) const { return { vec + other.vec }; }
	inline vec4f	operator+(const vec3f other) const { return { vec + other.vec }; }
	inline vec4f	operator-(const vec4f other) const { return { vec - other.vec }; }
	inline vec4f	operator/(const vec4f other) const { return { vec / other.vec }; }
	inline vec4f	operator/(const float f)const { return { vec / from(f).vec }; }
	inline vec4f	operator*(const vec4f other)const
	{
		__m128 v = vec * other.vec;
		return { v };
	}
	inline vec4f	operator*(const float num) const { return { vec * from(num).vec }; }
	inline void	operator+=(const vec4f other) { vec += other.vec; }
	inline void	operator+=(const vec3f other) { vec += other.vec; }
	inline void	operator-=(const vec4f other) { vec -= other.vec; }
	inline void	operator/=(const vec4f other) { vec /= other.vec; }
	inline void	operator*=(const vec4f other) { vec *= other.vec; }
	inline vec4f	operator-()const { return { -vec}; }
	inline float	x()const { return *(float*)&vec; }
	inline float	y()const { return *((float*)&vec + 1); }
	inline float	z()const { return *((float*)&vec + 2); }
	inline float	w()const { return *((float*)&vec + 3); }

	template<int mx = 0, int my = 0, int mz = 0, int mw = 0>
	inline vec4f	mask() const
	{
		__v4si mask = { mx ? -1 : mx,
						my ? -1 : my,
						mz ? -1 : mz,
						mw ? -1 : mw, };
		return { _mm_and_ps(vec, (__m128)mask) };
	}

	/// returns true if these vector are equal.
	inline bool operator==(const vec4f other) const
	{
		int mask =_mm_movemask_ps(_mm_cmpeq_ps(vec, other.vec));
		return (mask == 0xF);
	}

	inline bool operator!=(const vec4f other)const { return not (*this == other); }

	inline vec4f abs() const
	{
		__m128i tmp = _mm_castps_si128(vec);
		tmp = _mm_slli_epi32(tmp, 1);
		tmp = _mm_srli_epi32(tmp, 1);
		return { _mm_castsi128_ps(tmp) };
	}

	inline vec4f floor() const
	{
		#ifdef __SSE4__
		return _mm_floor_ps(vec);
		#else
		__m128 v1 = _mm_cvtepi32_ps(_mm_cvttps_epi32(vec)); // remove fractionals
		__m128 tmp = vec - v1; // store fractionals in vec

		// convert fractionals to 0 or 1
		tmp = _mm_cvtepi32_ps(_mm_srli_epi32(_mm_castps_si128(tmp), 31));
		v1 = v1 - tmp;
		return { v1 };
		#endif
	}

	inline vec4f ceil() const
	{
		#ifdef __SSE4__
		return { _mm_ceil_ps(vec) };
		#else
		__m128 tmp = _mm_cvtepi32_ps(_mm_cvttps_epi32(vec)); // remove fractionals
		tmp += _mm_cvtepi32_ps(_mm_srli_epi32(_mm_castps_si128(tmp - vec), 31));
		return { tmp };
		#endif
	}


	/// creates a permutation fo this vector.
	template<unsigned m0, unsigned m1, unsigned m2, unsigned m3>
	inline vec4f shuffle() const
	{
		static_assert(m0 < 4 and m1 < 4 and m2 < 4 and m3 < 4, "bad shuffle index");
		return { sse::shuffle<m0, m1, m2, m3>(vec) };
	}

	/// Adds all the elements of the vector and stores the result in each component
	inline vec4f hsum() const
	{
		#ifdef __SSE3__
		__m128 tmp = _mm_hadd_ps(vec, vec);
		return { _mm_hadd_ps(tmp, tmp) };
		#else

		vec4f v = *this + shuffle<1, 0, 3, 2>();
		// v is { 0 + 1, 1 + 0, 2 + 3, 3 + 2 }
		return v + v.shuffle<2, 3, 0, 1>();
		#endif
	}

	/// horizontally adds all components and puts the result into x, other components are undefined.
	inline vec4f hsum1() const
	{
		return hsum();
	}

	/// square root of x
	inline vec4f sqrt1() const
	{
		return { _mm_sqrt_ss(vec) };
	}

	/// square root in all components
	inline vec4f sqrt() const
	{
		return { _mm_sqrt_ps(vec) };
	}

	/// computes a squared length of this vector and puts it in each component of this vector
	inline float lengthSquared() const
	{
		return (*this * *this).hsum1().x();
	}

	/// computes a length(magnitude) of this vector and puts it in each component of this vector
	inline float length() const
	{
		vec4f t = { (vec * vec) };
		return t.hsum1().sqrt1().x();
	}

	/// computes normalized version of this vector.
	inline vec4f normalize() const
	{
		return *this / length();
	}

	/*
	template<unsigned char components>
	inline vec4f shiftComponentsRight() const
	{
		return { SHIFT_BYTES_RIGHT(vec, components * 4) };
	}

	template<unsigned char components>
	inline vec4f shiftComponentsLeft() const
	{
		return { SHIFT_BYTES_LEFT(vec, components * 4) };
	}
	*/
	/// creates a vec4f from a vector @param v @param component
	template<unsigned c>
	inline vec4f fromComponent() const
	{
		static_assert((c < 4), "bad component index");
		return shuffle<c, c, c, c>();
	}

	/// create a vec3f from a floating point number.
	inline static vec4f from(const float a)
	{
		return { _mm_set1_ps(a) };
	}

	/// create a vec3f from a 3 dimensional vector.
	inline static vec4f from(const vec3f a)
	{
		return { a.vec };
	}

	/// create a vec3f from a 3 dimensional vector and a floating point w component.
	inline static vec4f from(const vec3f a, const float w)
	{
		vec4f v = from(a);
		return v + from000W(w);
	}

	/// create a vector from a floating point numbers.
	inline static vec4f from(const float x, const float y, const float z = 0., const float w = 0.)
	{
		return { _mm_set_ps(w, z, y, x) };
	}

	/// Creates a vector @param x and zeroes the rest.
	inline static vec4f fromX000(const float x)
	{
		return { _mm_load_ss(&x) };
	}

	/// Creates a vector with @param w component and zeroes the rest.
	inline static vec4f from000W(const float w)
	{
		return fromX000(w).shuffle<1, 1, 1, 0>();
	}

    inline static vec4f from(const float* ptr)
    {
        if (ptr & 0x0F) // unaligned address
            return vec4f::from(ptr[0], ptr[1], ptr[2], ptr[3])
        else
            return *(vec4f*)ptr;
    }

	inline vec3f toVec3f() const
	{
		return { mask<1, 1, 1, 0>().vec };
	}

};
#pragma pack()

/// dot product of a and b. The result is in each component
template<class T>
inline T dotVec(T a, T b)
{
	#ifdef __SSE4__
	/*
	tmp0 := (mask4 == 1) ? (a0 * b0) : +0.0
	tmp1 := (mask5 == 1) ? (a1 * b1) : +0.0
	tmp2 := (mask6 == 1) ? (a2 * b2) : +0.0
	tmp3 := (mask7 == 1) ? (a3 * b3) : +0.0

	tmp4 := tmp0 + tmp1 + tmp2 + tmp3

	r0 := (mask0 == 1) ? tmp4 : +0.0
	r1 := (mask1 == 1) ? tmp4 : +0.0
	r2 := (mask2 == 1) ? tmp4 : +0.0
	r3 := (mask3 == 1) ? tmp4 : +0.0
	*/

	return _mm_dp_ps(a.vec, b.vec, 0xFF);
	#else
	return (a * b).hsum();
	#endif
}

/// dot product of a and b. The result is in each component
template<class T>
inline float dot(T a, T b)
{
	#ifdef __SSE4__
	/*
	tmp0 := (mask4 == 1) ? (a0 * b0) : +0.0
	tmp1 := (mask5 == 1) ? (a1 * b1) : +0.0
	tmp2 := (mask6 == 1) ? (a2 * b2) : +0.0
	tmp3 := (mask7 == 1) ? (a3 * b3) : +0.0

	tmp4 := tmp0 + tmp1 + tmp2 + tmp3

	r0 := (mask0 == 1) ? tmp4 : +0.0
	r1 := (mask1 == 1) ? tmp4 : +0.0
	r2 := (mask2 == 1) ? tmp4 : +0.0
	r3 := (mask3 == 1) ? tmp4 : +0.0
	*/

	vec4f v = { _mm_dp_ps(a.vec, b.vec, 0x1F) };
	return v.x();
	#else
	return (a * b).hsum1().x();
	#endif
}

inline vec4f cross(const vec4f v1, const vec4f v2)
{
	return	v1.shuffle<1, 2, 0, 3>() * v2.shuffle<2, 0, 1, 3>() -
			v1.shuffle<2, 0, 1, 3>() * v2.shuffle<1, 2, 0, 3>();
}

inline vec3f cross(const vec3f v1, const vec3f v2)
{
	return	v1.shuffle<1, 2, 0>() * v2.shuffle<2, 0, 1>() -
			v1.shuffle<2, 0, 1>() * v2.shuffle<1, 2, 0>();
}


/// Computes a vector of maximums of the components of @param v1 and @param v2
inline vec4f vecmax(const vec4f v1, const vec4f v2)
{
	return { _mm_max_ps(v1.vec, v2.vec) };
}

inline vec4f vecmin(const vec4f v1, const vec4f v2)
{
	return { _mm_min_ps(v1.vec, v2.vec) };
}

/// Computes a vector of maximums of the components of @param v1 and @param v2
inline vec3f vecmax(const vec3f v1, const vec3f v2)
{
	return { _mm_max_ps(v1.vec, v2.vec) };
}

inline vec3f vecmin(const vec3f v1, const vec3f v2)
{
	return { _mm_min_ps(v1.vec, v2.vec) };
}


inline vec4f vec3f::toVec4f() const { return { vec }; }
