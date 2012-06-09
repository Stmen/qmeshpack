#pragma once
#include "common-vec.h"

#define m00 elem[0]
#define m10 elem[1]
#define m20 elem[2]
#define m30 elem[3]
#define m01 elem[4]
#define m11 elem[5]
#define m21 elem[6]
#define m31 elem[7]
#define m02 elem[8]
#define m12 elem[9]
#define m22 elem[10]
#define m32 elem[11]
#define m03 elem[12]
#define m13 elem[13]
#define m23 elem[14]
#define m33 elem[15]

///////////////////////////////////////////////////////////////////////////////
//
//    Returns true if v1 is nearly equal v2
//
///////////////////////////////////////////////////////////////////////////////
template<class T1, class T2>
inline bool nearlyEqual(T1 v1, T2 v2)
{
	T1 diff = fabs(v1 - v2);
	if(diff > SMALL_NUMBER)
		return false;

	return true;
}

template<>
inline bool nearlyEqual(vec3f v1, vec3f v2)
{
	vec3f diff = v1 - v2;
	const float* elem = (const float*)&diff.vec;
	for(unsigned i = 0; i < 3; i++)
	{
		float f = fabs(elem[i]);
		if(f > SMALL_NUMBER)
			return false;
	}
	return true;
}

template<>
inline bool nearlyEqual(vec4f v1, vec4f v2)
{
	vec4f diff = v1 - v2;
	const float* elem = (const float*)&diff.vec;
	for(unsigned i = 0; i < 4; i++)
	{
		float f = fabs(elem[i]);
		if(f > SMALL_NUMBER)
			return false;
	}
	return true;
}

#pragma pack(16)
/////////////////////////////////////////////////////////////////////////////////
///
///  This class implements Column Major 3x3 Matrix. Because of SIMD, the basis
///  vectors have are actually 4 elements.
///
///    0 4 8
///    1 5 9
///    2 6 10
///    3 7 11
///
/// The rows of R represent the coordinates in the original space of unit vectors
/// along the coordinate axes of the rotated space.
/// The columns of R represent the coordinates in the rotated space of unit
/// vectors along the axes of the original space.
/// Then row1 is right, row2 is up and row3 is out.
///
/////////////////////////////////////////////////////////////////////////////////
struct mat3f
{
	vec4f vec[3];

	SIMD_NEW_DELETE;

	/// Constructs a Matrix from a diagonal
	inline static mat3f from(float num)
	{
		mat3f result;
		result.vec[0] = vec4f::fromX000(num);
		result.vec[1] = result.vec[0].shuffle<3, 0, 1, 2>();
		result.vec[2] = result.vec[1].shuffle<3, 0, 1, 2>();
		return result;
	}

	inline static mat3f fromXAxisAngle(float angle)
	{
		float s = sin(angle);
		float c = cos(angle);
		return {{
			vec4f::from(1., 0., 0.,	0.),
			vec4f::from(0., c,	s,	0.),
			vec4f::from(0.,-s,	c,	0.) }};
	}

	inline static mat3f fromYAxisAngle(float angle)
	{
		float s = sin(angle);
		float c = cos(angle);
		return {{
			vec4f::from(c,	0.,	-s,	0.),
			vec4f::from(0.,	1,	0,	0.),
			vec4f::from(s,	0.,	c,	0.) }};
	}

	inline static mat3f fromZAxisAngle(float angle)
	{
		float s = sin(angle);
		float c = cos(angle);
		return {{
			vec4f::from(c,	s,	0.,	0.),
			vec4f::from(-s,	c,	0.,	0.),
			vec4f::from(0.,	0.,	1.,	0.) }};
	}

	inline static mat3f fromAxisAngle(vec3f axis, float angle)
	{
		assert(nearlyEqual(axis.lengthSquared(), 1.f));


		vec4f a = axis.toVec4f();
		float ca = cos(angle);
		vec4f sv = vec4f::from(sin(angle));
		vec4f c = vec4f::fromX000(ca);
		vec4f m1ca = vec4f::from(1. - ca) * a;
		vec4f mask = vec4f::from(0., 1., -1., 0.);

		return {{
			c +							m1ca * a.fromComponent<0>() + sv * mask * a.shuffle<0, 2, 1, 3>(),
			c.shuffle<1, 0, 1, 1>() +	m1ca * a.fromComponent<1>() + sv * mask.shuffle<2, 0, 1, 3>() * a.shuffle<2, 3, 0, 1>(),
			c.shuffle<1, 1, 0, 1>() +	m1ca * a.fromComponent<2>() + sv * mask.shuffle<1, 2, 0, 3>() * a.shuffle<1, 0, 2, 3>() }};
	}

	inline static mat3f fromEulerAnglesZYX(float x, float y, float z)
	{
		float sinA = sin(x);
		float sinB = sin(y);
		float sinC = sin(z);

		float cosA = cos(x);
		float cosB = cos(y);
		float cosC = cos(z);

		vec4f a = vec4f::from(sinA, cosA, -cosA, 1.);
		vec4f b = vec4f::from(sinB, cosB, -cosB, 0.);
		vec4f c = vec4f::from(sinC, cosC, -sinC, 0.);

		mat3f m = {{
			// (1, sinA, -cosA, *)   * (cosB, sinB, sinB, 0) *  (cosC, cosC, cosC, *)
			a.shuffle<3, 0, 2, 1>() * b.shuffle<1, 0, 0, 3>() * c.fromComponent<1>() +
			//   (*, cosA, sinA, *) * (0, cosC, cosC, 0)
			a.shuffle<1, 1, 0, 0>() * c.shuffle<3, 0, 0, 3>(),

			//  (1, sinA, -cosA, *) *   (cosC, sinC, sinC, 0) * (-sinC, -sinC, -sinC, *)
			a.shuffle<3, 0, 2, 2>() * b.shuffle<1, 0, 0, 3>() * c.fromComponent<2>() +
			//  (*, cosA, sinA, *) *       (0, cosC, cosC, 0)
			a.shuffle<1, 1, 0, 0>() * c.shuffle<3, 1, 1, 3>(),

			// (1, sinA, cosA, *)	*  (0, -cosB, cosB, 0)
			a.shuffle<3, 0, 1, 3>() * b.shuffle<0, 2, 1, 3>(),

		}};
		return m;
	}

	///////////////////////////////////////////////////////////////////////////////
	//
	//  Converts quaternion to a 3x3 matrix.
	//
	//  Algorith used:
	//
	//        float x2 = x * x;
	//        float y2 = y * y;
	//        float z2 = z * z;
	//
	//        float xy = x * y;
	//        float yz = y * z;
	//        float zx = z * x;
	//
	//        float wx = w * x;
	//        float wy = w * y;
	//        float wz = w * z;
	//
	//        mat.elem[0] = 1.0f - 2.0f * (y2 + z2);
	//        mat.elem[1] =        2.0f * (xy - wz);
	//        mat.elem[2] =        2.0f * (zx + wy);
	//        mat.elem[3] =        0.0f;
	//
	//        mat.elem[4] =        2.0f * (xy + wz);
	//        mat.elem[5] = 1.0f - 2.0f * (x2 + z2);
	//        mat.elem[6] =        2.0f * (yz - wx);
	//        mat.elem[7] =        0.0f;
	//
	//        mat.elem[8] =         2.0f * (zx - wy);
	//        mat.elem[9] =         2.0f * (yz + wx);
	//        mat.elem[10] = 1.0f - 2.0f * (x2 + y2);
	//        mat.elem[11] =        0.0f;
	//
	///////////////////////////////////////////////////////////////////////////////
	static mat3f from(const quatf q);

	/// returns true if this is a rotation matrix
	bool isRotation() const
	{
		for (unsigned i = 0; i < 3; i++)
		{
			assert(vec[i].toVec3f().errorCheck());

			float length = vec[i].lengthSquared();
			if((length - 1.) > SMALL_NUMBER)
			{
				return false;
			}
		}
		float det = abs(det1().x());
		if((det - 1.) > SMALL_NUMBER)
			return false;
		return true;
	}

	/// sets a column vector of this matrix
	inline void setColumn(unsigned idx, vec3f value)
	{
		assert(idx < 3);
		vec[idx] = vec4f::from(value);
	}

	inline const float* element() const
	{
		return (float*) this;
	}

	inline  void setElem(unsigned idx, const float val)
	{
		assert(idx < 12);
		float* f = (float*) this;
		f[idx] = val;
	}

	inline  static mat3f identity()
	{
		return from(1.);
	}

	/// multiply element wise
	inline mat3f operator*(const float factor) const
	{
		vec4f vec_factor = vec4f::from(factor);
		return {{ vec[0] * vec_factor, vec[1] * vec_factor, vec[2] * vec_factor }};
	}

	bool operator==(const mat3f other) const
	{
		return	vec[0] == other.vec[0] and
				vec[1] == other.vec[1] and
				vec[2] == other.vec[2];
	}

	bool nearEqual(const mat3f& other) const
	{
		return nearlyEqual(vec[0], other.vec[0]) and
			   nearlyEqual(vec[1], other.vec[0]) and
			   nearlyEqual(vec[2], other.vec[0]);
	}

	/// divide element wise
	inline mat3f operator/(const float f) const
	{
		vec4f vec_divisor = vec4f::from(f);
		return {{ vec[0] / vec_divisor, vec[1] / vec_divisor, vec[2] / vec_divisor }};
	}

	/// matrix multiplication
	inline mat3f operator*(const mat3f& mat) const
	{
		return { {
			vec[0] * mat.vec[0].fromComponent<0>() +
			vec[1] * mat.vec[0].fromComponent<1>() +
			vec[2] * mat.vec[0].fromComponent<2>(),

			vec[0] * mat.vec[1].fromComponent<0>() +
			vec[1] * mat.vec[1].fromComponent<1>() +
			vec[2] * mat.vec[1].fromComponent<2>(),

			vec[0] * mat.vec[2].fromComponent<0>() +
			vec[1] * mat.vec[2].fromComponent<1>() +
			vec[2] * mat.vec[2].fromComponent<2>() } };
	}

	/// same as above, only in-place
	inline void operator*=(const mat3f& mat)
	{
		*this = *this * mat;
	}

	/// multiply this matrix by a vector
	inline vec3f operator*(vec3f v) const
	{
		vec4f v4 = vec4f::from(v);
		return (vec[0] * v4.fromComponent<0>() +
				vec[1] * v4.fromComponent<1>() +
				vec[2] * v4.fromComponent<2>()).toVec3f();
	}

	/// scales this matrix by f
	inline  mat3f scale(const float f) const
	{
		return *this * from(f);
	}

	/// converts this matrix to quaternion.
	inline quatf toQuat() const
	{
		const float* elem = element();

		float tr = m00 + m11 + m22;
		if (tr > 0)
		{
			float S = sqrt(tr+1.0) * 2; // S=4*qw
			return quatf::from((m21 - m12) / S, (m02 - m20) / S, (m10 - m01) / S, 0.25 * S);
		}
		else if ((m00 > m11)&(m00 > m22))
		{
			float S = sqrt(1.0 + m00 - m11 - m22) * 2; // S=4*qx
			return quatf::from(0.25 * S, (m01 + m10) / S, (m02 + m20) / S, (m21 - m12) / S);
		} else if (m11 > m22)
		{
			float S = sqrt(1.0 + m11 - m00 - m22) * 2; // S=4*qy
			return quatf::from((m01 + m10) / S, 0.25 * S, (m12 + m21) / S, (m02 - m20) / S);
		} else
		{
			float S = sqrt(1.0 + m22 - m00 - m11) * 2; // S=4*qz
			return quatf::from((m02 + m20) / S, (m12 + m21) / S, 0.25 * S, (m10 - m01) / S);
		}
	}

	mat3f transpose() const;

	inline mat3f normalize() const
	{
		mat3f m;
		m.vec[2] = vec[2].normalize();
		m.vec[0] = cross(vec[1], m.vec[2]);
		m.vec[0] = m.vec[0].normalize();
		m.vec[1] = cross(m.vec[2], m.vec[0]);
		return m;
	}

	inline void normalizeInPlace()
	{
		vec[2] = vec[2].normalize();
		vec[0] = cross(vec[1], vec[2]);
		vec[0] = vec[0].normalize();
		vec[1] = cross(vec[2], vec[0]);
	}

	// returns determinant of this 3x3 matrix
	inline vec4f det1() const
	{
		return	(vec[0] * vec[1].shuffle<1, 2, 0, 3>() * vec[2].shuffle<2, 0, 1, 3>()).hsum1() -
				(vec[0] * vec[1].shuffle<2, 0, 1, 3>() * vec[2].shuffle<1, 2, 0, 3>()).hsum1();

	}

	// computes inverse of this 3x3 matrix.
	inline mat3f inverse() const
	{
		vec4f b1_312 = vec[0].shuffle<2, 0, 1, 3>();
		vec4f b1_231 = vec[0].shuffle<1, 2, 0, 3>();
		vec4f b2_231 = vec[1].shuffle<1, 2, 0, 3>();
		vec4f b2_312 = vec[1].shuffle<2, 0, 1, 3>();
		vec4f b3_312 = vec[2].shuffle<2, 0, 1, 3>();
		vec4f b3_231 = vec[2].shuffle<1, 2, 0, 3>();

		mat3f m = {{b2_231 * b3_312 - b2_312 * b3_231,
					b1_312 * b3_231 - b1_231 * b3_312,
					b1_231 * b2_312 - b1_312 * b2_231 }};

		float det = ((vec[0] * b2_231 * b3_312).hsum1() - (vec[0] * b2_312 * b3_231).hsum1()).x();
		assert(abs(det) > SMALL_NUMBER);
		return m / det;

	}
};


///////////////////////////////////////////////////////////////////////////////
///
///  This class implements Column Major 4x4 Matrix.
///  Element position looks like this:  matrix with vec[3] = (x,y,z,1)
///
///    0 4 8  12         1 0 0 x
///    1 5 9  13         0 1 0 y
///    2 6 10 14         0 0 1 z
///    3 7 11 15         0 0 0 1
///
///////////////////////////////////////////////////////////////////////////////
struct mat4f
{
	vec4f vec[4];

	SIMD_NEW_DELETE;


	///////////////////////////////////////////////////////////////////////////////
	///
	/// Constructs a Matrix from a diagonal
	///
	///////////////////////////////////////////////////////////////////////////////
	inline static mat4f from(float num)
	{
		mat4f result;
		result.vec[0] = vec4f::fromX000(num);
		result.vec[1] = result.vec[0].shuffle<3, 0, 1, 2>();
		result.vec[2] = result.vec[1].shuffle<3, 0, 1, 2>();
		result.vec[3] = result.vec[2].shuffle<3, 0, 1, 2>();
		return result;
	}

	///////////////////////////////////////////////////////////////////////////////
	///
	/// Constructs a Matrix from a diagonal
	///
	///////////////////////////////////////////////////////////////////////////////
	inline static mat4f from(const mat4f& other)
	{
		return other;
	}

	///////////////////////////////////////////////////////////////////////////////
	///
	/// Constructs a Matrix from a diagonal
	///
	///////////////////////////////////////////////////////////////////////////////
	inline static mat4f from(const mat3f& other)
	{
		return {{ other.vec[0], other.vec[1], other.vec[2], vec4f::from000W(1.) }};
	}


	///////////////////////////////////////////////////////////////////////////////
	//
	//  Converts quaternion to a 3x3 matrix.
	//
	//  Algorith used:
	//
	//        float x2 = x * x;
	//        float y2 = y * y;
	//        float z2 = z * z;
	//
	//        float xy = x * y;
	//        float yz = y * z;
	//        float zx = z * x;
	//
	//        float wx = w * x;
	//        float wy = w * y;
	//        float wz = w * z;
	//
	//        mat.elem[0] = 1.0f - 2.0f * (y2 + z2);
	//        mat.elem[1] =        2.0f * (xy - wz);
	//        mat.elem[2] =        2.0f * (zx + wy);
	//        mat.elem[3] =        0.0f;
	//
	//        mat.elem[4] =        2.0f * (xy + wz);
	//        mat.elem[5] = 1.0f - 2.0f * (x2 + z2);
	//        mat.elem[6] =        2.0f * (yz - wx);
	//        mat.elem[7] =        0.0f;
	//
	//        mat.elem[8] =         2.0f * (zx - wy);
	//        mat.elem[9] =         2.0f * (yz + wx);
	//        mat.elem[10] = 1.0f - 2.0f * (x2 + y2);
	//        mat.elem[11] =        0.0f;
	//
	///////////////////////////////////////////////////////////////////////////////
	static mat4f from(const quatf q);

	inline void setVec(unsigned idx, vec4f value)
	{
		vec[idx] = value;
	}

	inline const float* element() const
	{
		return (float*) this;
	}

	inline  void setElem(unsigned idx, const float val)
	{
		float* f = (float*) this;
		f[idx] = val;
	}

	inline  static mat4f identity()
	{
		return from(1.);
	}

	mat4f transpose() const;

	inline mat4f operator*(const mat4f& mat) const
	{
		mat4f result;

		/* Dont make this into a loop, because gcc(as of 4.5.0) tries to unroll
		 it and failes completely to properly compute dependencies with -O2.
		 */

		result.vec[0] =
			vec[0] * mat.vec[0].fromComponent<0>() +
			vec[1] * mat.vec[0].fromComponent<1>() +
			vec[2] * mat.vec[0].fromComponent<2>() +
			vec[3] * mat.vec[0].fromComponent<3>();

		result.vec[1] =
			vec[0] * mat.vec[1].fromComponent<0>() +
			vec[1] * mat.vec[1].fromComponent<1>() +
			vec[2] * mat.vec[1].fromComponent<2>() +
			vec[3] * mat.vec[1].fromComponent<3>();

		result.vec[2] =
			vec[0] * mat.vec[2].fromComponent<0>() +
			vec[1] * mat.vec[2].fromComponent<1>() +
			vec[2] * mat.vec[2].fromComponent<2>() +
			vec[3] * mat.vec[2].fromComponent<3>();

		result.vec[3] =
			vec[0] * mat.vec[3].fromComponent<0>() +
			vec[1] * mat.vec[3].fromComponent<1>() +
			vec[2] * mat.vec[3].fromComponent<2>() +
			vec[3] * mat.vec[3].fromComponent<3>();
		return result;
	}

	inline void operator*=(const mat4f& mat)
	{
		*this = *this * mat;
	}

	inline vec4f operator*(vec4f v) const
	{
		return	vec[0] * v.fromComponent<0>() +
				vec[1] * v.fromComponent<1>() +
				vec[2] * v.fromComponent<2>() +
				vec[3] * v.fromComponent<3>();
	}

	inline mat4f operator*(float factor) const
	{
		mat4f result;
		for(unsigned i = 0; i < 4; i++)
			result.vec[i] = vec[i] * vec4f::from(factor);

		return result;
	}

	inline mat4f operator/(float divisor) const
	{
		mat4f result;
		for(unsigned i = 0; i < 4; i++)
			result.vec[i] = vec[i] * vec4f::from(divisor);

		return result;
	}

	vec4f blub1(unsigned i1, unsigned i2, unsigned i3) const
	{
		return
				vec[i1].shuffle<1, 0, 0, 0>() * vec[i2].shuffle<2, 3, 1, 2>() * vec[i3].shuffle<3, 2, 3, 1>() +
				vec[i1].shuffle<2, 2, 1, 1>() * vec[i2].shuffle<3, 0, 3, 0>() * vec[i3].shuffle<1, 3, 0, 2>() +
				vec[i1].shuffle<3, 3, 3, 2>() * vec[i2].shuffle<1, 2, 0, 1>() * vec[i3].shuffle<2, 0, 1, 0>();
	}

	vec4f blub2(unsigned i1, unsigned i2, unsigned i3) const
	{
		return
				vec[i1].shuffle<1, 0, 0, 0>() * vec[i2].shuffle<3, 2, 3, 1>() * vec[i3].shuffle<2, 3, 1, 2>() +
				vec[i1].shuffle<2, 2, 1, 1>() * vec[i2].shuffle<1, 3, 0, 2>() * vec[i3].shuffle<3, 0, 3, 0>() +
				vec[i1].shuffle<3, 3, 3, 2>() * vec[i2].shuffle<2, 0, 1, 0>() * vec[i3].shuffle<1, 2, 0, 1>();
	}

	float det() const
	{
		mat4f t = transpose();

		return (t.vec[0] * (t.blub1(1, 2, 3) - t.blub2(1, 2, 3))).hsum1().x();
	}

	inline mat4f inverse() const
	{
		float d = det();
		assert(abs(d) > SMALL_NUMBER);
		mat4f b;
		b.vec[0] = blub1(1, 2, 3) - blub2(1, 2, 3);
		b.vec[1] = blub2(0, 2, 3) - blub1(0, 2, 3);
		b.vec[2] = blub1(0, 1, 3) - blub2(0, 1, 3);
		b.vec[3] = blub2(0, 1, 2) - blub1(0, 1, 2);
		return b / d;
	}

	inline void translate(const vec3f offset)
	{
		mat4f result = from(1.);
		result.vec[3] += vec4f::from(offset);
		*this = *this * result;
	}

	inline mat4f scale(const float f) const
	{
		return *this * from(f);
	}

	inline mat4f invertRotTrans() const
	{
		mat4f result = { { vec[0], vec[1], vec[2], vec[3].mask<0, 0, 0, 1>() } };
		result = result.transpose();
		result.vec[3] = -(result * vec[3]);
		return result;
	}

	inline quatf toQuat() const
	{
		const float* elem = element();


		float tr = m00 + m11 + m22;
		if (tr > 0)
		{
			float S = sqrt(tr+1.0) * 2; // S=4*qw
			return quatf::from((m21 - m12) / S, (m02 - m20) / S, (m10 - m01) / S, 0.25 * S);
		}
		else if ((m00 > m11)&(m00 > m22))
		{
			float S = sqrt(1.0 + m00 - m11 - m22) * 2; // S=4*qx
			return quatf::from(0.25 * S, (m01 + m10) / S, (m02 + m20) / S, (m21 - m12) / S);
		} else if (m11 > m22)
		{
			float S = sqrt(1.0 + m11 - m00 - m22) * 2; // S=4*qy
			return quatf::from((m01 + m10) / S, 0.25 * S, (m12 + m21) / S, (m02 - m20) / S);
		} else
		{
			float S = sqrt(1.0 + m22 - m00 - m11) * 2; // S=4*qz
			return quatf::from((m02 + m20) / S, (m12 + m21) / S, 0.25 * S, (m10 - m01) / S);
		}
	}

};
