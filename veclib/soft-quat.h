#pragma once
#include "soft-vec.h"

struct quatf
{
	v4f vec;

	inline float 	x() const { return ((float*)&vec)[0]; }
	inline float 	y() const { return ((float*)&vec)[1]; }
	inline float 	z() const { return ((float*)&vec)[2]; }
	inline float 	w() const { return ((float*)&vec)[3]; }

	inline bool operator==(quatf other)const
	{
		return  x() == other.x() and y() == other.y() and z() == other.z() and w() == other.w();
	}

	inline bool operator!=(quatf other)const { return not (*this == other); }

	/// creates a permutation fo this vector.
	template<unsigned m0, unsigned m1, unsigned m2, unsigned m3>
	inline quatf shuffle() const
	{
		static_assert((m0 < 4 and m1 < 4 and m2 < 4 and m3 < 4), "bad component index");
		const float* f = (const float*)&vec;
		v4f r = { f[m0], f[m1], f[m2], f[m3] };
		return { { r } };
	}

	/// creates a unit quaternion
	inline static quatf identity()
	{
		return { vec4f::from000W(1.).vec };
	}

	inline quatf conj() const
	{
		return { vec * vec4f::from(-1., -1., -1., 1).vec };
	}

	/// computes normalized version of this vector.
	inline quatf normalize() const
	{
		vec4f v = { vec };
		return { v.normalize().vec };
	}

	inline vec3f toVec3f() const
	{
		vec3f r = { vec };
		return r.mask<1, 1, 1>();
	}

	inline vec4f toVec4f() const
	{
		return { vec };
	}

	///////////////////////////////////////////////////////////////////////////////
	//
	//  Performs quaternion multiplication
	//
	//   y * rz + x * rw + w * rx - z * ry,
	//   w * ry + z * rx + y * rw - x * rz,
	//   z * rw + w * rz + x * ry - y * rx,
	// -(x * rx + y * ry + z * rz - w * rw)
	//
	///////////////////////////////////////////////////////////////////////////////
	template <class T>
	inline quatf operator * (const T other) const
	{
		vec4f v1 = { vec }, v2 = { other.vec };

		vec4f r =	v1.shuffle<1, 3, 2, 0>() * v2.shuffle<2, 1, 3, 0>() +
				v1.shuffle<0, 2, 3, 1>() * v2.shuffle<3, 0, 2, 1>() +
				v1.shuffle<3, 1, 0, 2>() * v2.shuffle<0, 3, 1, 2>() -
				v1.shuffle<2, 0, 1, 3>() * v2.shuffle<1, 2, 0, 3>();

		vec4f tmp = vec4f::from( 1., 1., 1., -1 );
		return { (r * tmp).vec };
	}

	inline static quatf from(float x, float y, float z, float w)
	{
		return { vec4f::from(x, y, z, w).vec };
	}

	///////////////////////////////////////////////////////////////////////////////
	//
	//  Creates a quaternion from a axis and an angle
	//
	//  qx = ax * sin(angle/2)
	//  qy = ay * sin(angle/2)
	//  qz = az * sin(angle/2)
	//  qw = cos(angle/2)
	//
	///////////////////////////////////////////////////////////////////////////////
	inline static quatf fromAxisAngle(const vec3f axis, const float angle)
	{
		assert((axis.length() - 1.0f < SMALL_NUMBER) && "Axis Vector is not normalized!");

		float sinHalf = std::sin(angle / 2.f);
		float cosHalf = std::cos(angle / 2.f);

		vec4f tmp = axis.toVec4f();

		return { (tmp * vec4f::from(sinHalf) + vec4f::from000W(cosHalf)).vec };
	}

	///////////////////////////////////////////////////////////////////////////////
	//
	//  Creates Quaternion from euler angles;
	//
	//  Algorithm used:
	//
	//  x = -sin_roll *  cos_pitch * sin_yaw  + sin_pitch * cos_yaw * cos_roll;
	//  y =  cos_roll *  cos_pitch * sin_yaw  + sin_pitch * cos_yaw * sin_roll;
	//  z =  cos_roll * -sin_pitch * sin_yaw  + cos_pitch * cos_yaw * sin_roll;
	//  w =  cos_roll *  cos_pitch * cos_yaw  + sin_pitch * sin_yaw * sin_roll;
	//
	///////////////////////////////////////////////////////////////////////////////
	inline static quatf fromEuler(float heading, float attitud, float bank)
	{
		 float c1 = cos(heading / 2);
		 float c2 = cos(bank / 2);
		 float c3 = cos(attitud / 2);
		 float s1 = sin(heading / 2);
		 float s2 = sin(bank / 2);
		 float s3 = sin(attitud / 2);

		 return { vec4f::from(s1*s2*c3 + c1*c2*s3, s1*c2*c3 + c1*s2*s3,
				 	 c1*s2*c3 - s1*c2*s3, c1*c2*c3 - s1*s2*s3).vec };
	}

	static quatf from(const vec4f v)
	{
		return { v.vec };
	}
};

template <>
inline quatf quatf::operator * (const vec3f v3) const
{
	quatf other = { v3.vec };
	return *this * other;
}

template <>
inline quatf quatf::operator * (const vec4f v3) const
{
	quatf other = { v3.vec };
	return *this * other;
}

