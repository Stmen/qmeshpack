#pragma once
#include "common-vec.h"
#include <cstddef>
#include <cstdlib>
typedef int		v4si __attribute__ ((vector_size (4 * sizeof(int))));
typedef float	v4d __attribute__ ((vector_size(4 * sizeof(double))));
typedef float	v4f __attribute__ ((vector_size(4 * sizeof(float))));

#define VEC_X 0
#define VEC_Y 1
#define VEC_Z 2
#define VEC_W 3
#pragma pack(16)

#define SIMD_NEW_DELETE
struct vec4f;

struct vec3f
{
	v4f vec;

	inline vec3f	operator+(vec3f other)const { return { vec + other.vec }; }
	inline vec3f	operator-(vec3f other)const { return { vec - other.vec }; }
	inline vec3f	operator/(vec3f other)const { vec3f v = { vec / other.vec }; v.clearW(); return v; }
	inline vec3f	operator/(float f)const { v4f v = {f, f, f, f}; return { vec / v }; }
	inline vec3f	operator*(vec3f other)const { return { vec * other.vec }; }
	inline vec3f	operator*(float num) const { return { vec * from(num).vec }; }
	inline void		operator+=(vec3f other) { vec += other.vec; }
	inline void		operator-=(vec3f other) { vec -= other.vec; }
	inline void		operator/=(vec3f other) { vec /= other.vec; }
	inline void		operator*=(vec3f other) { vec *= other.vec; }
	inline vec3f	operator-()const { return { -vec}; }
	inline float 	x()const { return *(float*)&vec; }
	inline float 	y()const { return *((float*)&vec + 1); }
	inline float 	z()const { return *((float*)&vec + 2); }
	inline float	elem(unsigned i)const { return *((float*)&vec + i); }
	inline bool		errorCheck() const { return elem(3) == 0.f; }
	inline void		clearW() { ((float*)&vec)[3] = 0.f; }

	inline bool operator==(vec3f other)const
	{
		return  x() == other.x() and
				y() == other.y() and
				z() == other.z();
	}

	inline bool operator!=(vec3f other)const { return not (*this == other); }

	template<unsigned mx = 0, unsigned my = 0, unsigned mz = 0>
	inline vec3f	mask() const
	{
		v4si vmask = { mx ? -1 : 0, my ? -1 : 0, mz ? -1 : 0, 0 };
		return { (v4f)((v4si)vec & vmask) };
	}

	inline vec3f abs()const { return from(::fabs(x()), ::fabs(y()), ::fabs(z())); }
	inline vec3f floor()const { return from(::floor(x()), ::floor(y()), ::floor(z())); }
	inline vec3f ceil()const { return from(::ceil(x()), ::ceil(y()), ::ceil(z())); }

	/// creates a permutation of this vector's components
	template<unsigned m0, unsigned m1, unsigned m2>
	inline vec3f shuffle() const
	{
		static_assert((m0 < 4 and m1 < 4 and m2 < 4), "bad component index");
		const float* f = (const float*)&vec;
		v4f r = { f[m0], f[m1], f[m2], 0.};
		return { r };
	}

	/// Adds all the elements of the vector and stores the result in each component
	inline vec3f hsum() const
	{
		return *this + shuffle<1, 2, 0>() + shuffle<2, 0, 1>();
	}


	/// horizontally adds all components and puts the result into x, other components are undefined
	inline vec3f hsum1() const
	{
		const float* value = (const float*)&vec;
		v4f r = { value[0] + value[1] + value[2], 0., 0., 0. };
		return { r };
	}

	inline vec3f sqrt1() const
	{
		vec3f v = { vec };
		float* x = (float*)&v.vec;
		*x = sqrt(*x);
		return v;
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

	/// shifts the components of this vector left by @param components
	template<unsigned char components>
	inline vec3f shiftComponentsRight() const
	{
		static_assert((components < 5), "bad component index");
		v4f v;
		float* value = (float*)&v;

		unsigned i = 0;
		for(; i < 3; i++)
		{
			if(i < components)
				value[i] = 0;
			if(i + components < 3)
				value[i + components] = elem(i);
		}
		value[3] = 0;

		return { v };
	}

	/// shifts the components of this vector left by @param components
	template<unsigned char components>
	inline vec3f shiftComponentsLeft() const
	{
		static_assert((components < 5), "bad component index");
		v4f v;
		float* value = (float*)&v;

		unsigned i = 0;
		for(; i < 3; i++)
		{
			if(i + components < 3)
				value[i] = elem(i + components);
			else
				value[i] = 0;
		}
		value[3] = 0;
		return { v };
	}

	/// Creates a vec3f from a vector @param v @param component
	template<unsigned c>
	inline vec3f fromComponent() const
	{
		static_assert((c < 3), "bad component index");
		return shuffle<c, c, c>();	}

	/// create a vector from a floating point numbers.
	inline static vec3f from(float x, float y, float z)
	{
		v4f r = {x, y, z, 0. };
		return { r };
	}

	/// create a v3f from a floating point number @param a
	inline static vec3f from(const float a)
	{
		v4f r = {a, a, a, 0. };
		return { r };
	}

    inline static vec3f from(const float* ptr)
    {
        return vec3f::from(ptr[0], ptr[1], ptr[2]);
    }

	/// Creates a vector @param x and zeroes the rest.
	inline static vec3f fromX00(const float x)
	{
		v4f r = {x, 0., 0., 0.};
		return { r };
	}

	vec4f toVec4f() const;

	struct packed
	{
		float elems[3];

		vec3f toVec3f() { vec3f r = { { elems[0], elems[1], elems[2], 0 } }; return r;  }
	};

	inline struct packed pack()
	{
		return { { x(), y(), z() } };
	}
};

struct vec4f
{
	v4f vec;

	inline vec4f	operator+(vec4f other)const { return { vec + other.vec }; }
	inline vec4f	operator+(vec3f other)const { return { vec + other.vec }; }
	inline vec4f	operator-(vec4f other)const { return { vec - other.vec }; }
	inline vec4f	operator/(vec4f other)const { return { vec / other.vec }; }
	inline vec4f	operator/(float f)const { return { vec / vec4f::from(f).vec }; }
	inline vec4f	operator*(vec4f other)const { return { vec * other.vec }; }
	inline vec3f	operator*(float num) const { return { vec * from(num).vec }; }
	inline void		operator+=(vec4f other) { vec += other.vec; }
	inline void		operator+=(vec3f other) { vec += other.vec; }
	inline void		operator+=(v4f other) { vec += other; }
	inline void		operator-=(vec4f other) { vec -= other.vec; }
	inline void		operator/=(vec4f other) { vec /= other.vec; }
	inline void		operator*=(vec4f other) { vec *= other.vec; }
	inline vec4f	operator-()const { return { -vec}; }
	inline float	x()const { return *(float*)&vec; }
	inline float	y()const { return *((float*)&vec + 1); }
	inline float	z()const { return *((float*)&vec + 2); }
	inline float	w()const { return *((float*)&vec + 3); }
	inline float	elem(unsigned i)const { return *((float*)&vec + i); }

	/// returns true if these vector are equal.
	inline bool operator==(vec4f other)const
	{
		return  x() == other.x() and
				y() == other.y() and
				z() == other.z() and
				w() == other.w();
	}

	inline bool operator!=(vec4f other)const { return not (*this == other); }

	template<int mx = 0, int my = 0, int mz = 0, int mw = 0>
	inline vec4f	mask() const
	{
		v4si vmask = {	mx ? -1 : 0,
						my ? -1 : 0,
						mz ? -1 : 0,
						mw ? -1 : 0, };
		return { (v4f)((v4si)vec & vmask) };
	}

	inline vec4f abs()const { return from(::fabs(x()), ::fabs(y()), ::fabs(z()), ::fabs(w())); }
	inline vec4f floor()const { return from(::floor(x()), ::floor(y()), ::floor(z()), ::floor(w())); }
	inline vec4f ceil()const { return from(::ceil(x()), ::ceil(y()), ::ceil(z()), ::ceil(w())); }

	/// creates a permutation fo this vector.
	template<unsigned idx0, unsigned idx1, unsigned idx2, unsigned idx3 = 3>
	inline vec4f shuffle() const
	{
		static_assert((idx0 < 4 and idx1 < 4 and idx2 < 4 and idx3 < 4), "bad component index");
		const float* f = (const float*)&vec;
		v4f r = { f[idx0], f[idx1], f[idx2], f[idx3] };
		return { r };
	}

	/// Adds all the elements of the vector and stores the result in each component
	inline vec4f hsum() const
	{
		vec4f v = *this + shuffle<1, 0, 3, 2>();
		// v is { 0 + 1, 1 + 0, 2 + 3, 3 + 2 }
		return v + v.shuffle<2, 3, 0, 1>();
	}

	/// horizontally adds all components and puts the result into x, other components are undefined.
	inline vec4f hsum1() const
	{
		return hsum();
	}

	/// computes a square root of the x component of this vector an puts it into x. Other components are undefined
	inline vec4f sqrt1() const
	{
		vec4f v = { vec };
		float* x = (float*)&v.vec;
		*x = sqrt(*x);
		return v;
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
	/// shifts the components of this vector left by @param components
	template<unsigned char components>
	inline vec4f shiftComponentsRight() const
	{
		static_assert((components < 5), "bad component index");
		v4f v;
		float* value = (float*)&v;

		unsigned i = 0;
		for(; i < 4; i++)
		{
			if(i < components)
				value[i] = 0;
			if(i + components < 4)
				value[i + components] = elem(i);
		}

		return { v };
	}

	/// shifts the components of this vector left by @param components
	template<unsigned char components>
	inline vec4f shiftComponentsLeft() const
	{
		static_assert((components < 5), "bad component index");
		v4f v;
		float* value = (float*)&v;

		unsigned i = 0;
		for(; i < 4; i++)
		{
			if(i + components < 4)
				value[i] = elem(i + components);
			else
				value[i] = 0;
		}

		return { v };
	}
*/
	/// creates a vec3f from a vector @param v @param component
	template <unsigned c>
	inline vec4f fromComponent() const
	{
		static_assert((c < 4), "bad component index");
		return shuffle<c, c, c, c>();
	}

	/// create a vec3f from a floating point number.
    inline static vec4f from(const float a)
	{
		v4f r = {a, a, a, a};
		return { r };
	}

	/// create a vec3f from a 3 dimensional vector.
    inline static vec4f from(const vec3f a)
	{
		return { a.vec };
	}

	/// create a vec3f from a 3 dimensional vector and a floating point w component.
    inline static vec4f from(const vec3f a, float w)
	{
		vec4f v = from(a);
		return v + from000W(w);
	}

	/// create a vector from a floating point numbers.
    inline static vec4f from(const float x, const float y, const float z = 0, const float w = 0.)
	{
		v4f r = {x, y, z, w};
		return { r };
	}

    inline static vec4f from(const float* ptr)
    {
        if ((ptrdiff_t)ptr & 0x0FUL) // unaligned address
            return vec4f::from(ptr[0], ptr[1], ptr[2], ptr[3]);
        else
            return *(vec4f*)ptr;
    }

	/// Creates a vector @param x and zeroes the rest.
	inline static vec4f fromX000(const float x)
	{
		v4f r = {x, 0., 0., 0.};
		return { r };
	}

	/// Creates a vector with @param w component and zeroes the rest.
	inline static vec4f from000W(const float w)
	{
		return fromX000(w).shuffle<1, 1, 1, 0>();
	}

	inline vec3f toVec3f() const
	{
		vec3f r = { vec };
		return r.mask<1, 1, 1>();
	}
};

/// dot product of a and b. The result is in each component
template<class T>
inline float dot(T a, T b)
{
	return (a * b).hsum1().x();
}

/// dot product of a and b. The result is in each component
template<class T>
inline T dotVec(T a, T b)
{
	return (a * b).hsum();
}


inline vec4f cross(vec4f v1, vec4f v2)
{
	return	v1.shuffle<1, 2, 0, 3>() * v2.shuffle<2, 0, 1, 3>() -
			v1.shuffle<2, 0, 1, 3>() * v2.shuffle<1, 2, 0, 3>();
}

inline vec3f cross(vec3f v1, vec3f v2)
{
	return	v1.shuffle<1, 2, 0>() * v2.shuffle<2, 0, 1>() -
			v1.shuffle<2, 0, 1>() * v2.shuffle<1, 2, 0>();
}

inline vec3f vecmin(const vec3f v1, const vec3f v2)
{
    return vec3f::from(v1.x() < v2.x() ? v1.x() : v2.x(),
                       v1.y() < v2.y() ? v1.y() : v2.y(),
                       v1.z() < v2.z() ? v1.z() : v2.z()
                       );
}

inline vec3f  vecmax(const vec3f v1, const vec3f v2)
{
    return vec3f::from(v1.x() > v2.x() ? v1.x() : v2.x(),
                       v1.y() > v2.y() ? v1.y() : v2.y(),
                       v1.z() > v2.z() ? v1.z() : v2.z()
                       );
}

inline vec4f vec3f::toVec4f() const { return { vec }; }


#pragma pack()

