#pragma once
#include "sse-vec.h"

#include <cassert>
#pragma pack(16)
struct quatf
{
	__m128 vec;

	SIMD_NEW_DELETE;

	inline float 	x()const { return *(float*)&vec; }
	inline float 	y()const { return *((float*)&vec + 1); }
	inline float 	z()const { return *((float*)&vec + 2); }
	inline float 	w()const { return *((float*)&vec + 3); }

	inline static quatf identity()
	{
		return { vec4f::from000W(1.).vec };
	}

	inline quatf conj() const
	{
		__v4si mask = { (int)(1U << 31), (int)(1U << 31), (int)(1U << 31), 0};
		return { _mm_xor_ps(vec, (__m128)mask) };
	}

	/// computes normalized version of this vector.
	inline quatf normalize() const
	{
		vec4f v = { vec };
		return { v.normalize().vec };
	}

	bool operator == (quatf other) const
	{
		vec4f v1 = { vec }, v2 = { other.vec };
		return v1 == v2;
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
		using namespace sse;
		__m128 result = shuffle<1, 3, 2, 0>(vec) * shuffle<2, 1, 3, 0>(other.vec) +
						shuffle<0, 2, 3, 1>(vec) * shuffle<3, 0, 2, 1>(other.vec) +
						shuffle<3, 1, 0, 2>(vec) * shuffle<0, 3, 1, 2>(other.vec) -
						shuffle<2, 0, 1, 3>(vec) * shuffle<1, 2, 0, 3>(other.vec);

		__v4si negW = {0, 0, 0, (int)(1 << 31)};
		return { _mm_xor_ps(result, (__m128)negW) };
	}

	// creates a vec3f from the real part of this quaternion.
	inline vec3f toVec3f() const
	{
		vec3f v = { vec };
		__v4si mask = { -1, -1, -1, 0 };
		return { _mm_and_ps(v.vec, (__m128)mask) };
	}

	/// create a vector from a floating point numbers.
	inline static quatf from(float x, float y, float z, float w)
	{
		quatf q = { { x, y, z, w } };
		return q;
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
		float l = axis.length();
		if(fabs(l - 1.0f) > SMALL_NUMBER)
		{
			assert(0 && "Axis Vector is not normalized!");
		}

		float sinHalf = std::sin(angle / 2.f);
		float cosHalf = std::cos(angle / 2.f);

		vec4f tmp = { axis.vec };
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
	inline static quatf fromEulerAngles(float yaw, float pitch, float roll)
	{
		float cos_pitch = 1.0f, cos_yaw = 1.0f, cos_roll = 1.0f, sin_pitch = 0.0f,
				sin_yaw = 0.0f, sin_roll = 0.0f;

		if (pitch)
		{
			sin_pitch = sin(pitch * 0.5f);
			cos_pitch = cos(pitch * 0.5f);
		}
		if (yaw)
		{
			sin_yaw = sin(yaw * 0.5f);
			cos_yaw = cos(yaw * 0.5f);
		}
		if (roll)
		{
			sin_roll = sin(roll * 0.5f);
			cos_roll = cos(roll * 0.5f);
		}

		__m128 vec;
		__m128 v1 = vec4f::from(-sin_roll, cos_pitch, sin_pitch, -sin_pitch).vec;
		__m128 v2 = vec4f::from(cos_roll, sin_roll, sin_yaw, cos_yaw).vec;

		vec = sse::fromComponent<0>(v2);
		vec = _mm_move_ss(vec, v1);

		vec *= sse::shuffle<1, 1, 3, 1>(v1);
		vec *= sse::shuffle<2, 2, 2, 3>(v2);

		__m128 tmp = sse::shuffle<2, 2, 1, 2>(v1);
		tmp *= sse::shuffle<3, 3, 3, 2>(v2);
		tmp *= sse::shuffle<0, 1, 1, 1>(v2);
		vec += tmp;

		return { vec };
	}

	static quatf from(const vec4f v)
	{
		return { v.vec };
	}

};

template <>
inline quatf quatf::operator * (const vec3f other) const
{
	return *this * other.toVec4f();
}

#pragma pack()

