#pragma once
#include "soft-quat.h"
#include "common-mat.h"

inline mat3f mat3f::from(const quatf q)
{
	float x = q.x();
	float y = q.y();
	float z = q.z();
	float w = q.w();

	float x2 = x * x;
	float y2 = y * y;
	float z2 = z * z;

	float xy = x * y;
	float yz = y * z;
	float zx = z * x;

	float wx = w * x;
	float wy = w * y;
	float wz = w * z;

	mat3f result;
	result.setElem(0, 1.0f - 2.0f * (y2 + z2));
	result.setElem(1, 2.0f * (xy - wz));
	result.setElem(2, 2.0f * (zx + wy));
	result.setElem(3, 0.0f);

	result.setElem(4, 2.0f * (xy + wz));
	result.setElem(5, 1.0f - 2.0f * (x2 + z2));
	result.setElem(6, 2.0f * (yz - wx));
	result.setElem(7, 0.0f);

	result.setElem(8, 2.0f * (zx - wy));
	result.setElem(9, 2.0f * (yz + wx));
	result.setElem(10, 1.0f - 2.0f * (x2 + y2));
	result.setElem(11, 0.);
	return result;
}

inline mat4f mat4f::from(const quatf q)
{
	float x = q.x();
	float y = q.y();
	float z = q.z();
	float w = q.w();

	float x2 = x * x;
	float y2 = y * y;
	float z2 = z * z;

	float xy = x * y;
	float yz = y * z;
	float zx = z * x;

	float wx = w * x;
	float wy = w * y;
	float wz = w * z;

	mat4f result;
	result.setElem(0, 1.0f - 2.0f * (y2 + z2));
	result.setElem(1, 2.0f * (xy - wz));
	result.setElem(2, 2.0f * (zx + wy));
	result.setElem(3, 0.0f);

	result.setElem(4, 2.0f * (xy + wz));
	result.setElem(5, 1.0f - 2.0f * (x2 + z2));
	result.setElem(6, 2.0f * (yz - wx));
	result.setElem(7, 0.0f);

	result.setElem(8, 2.0f * (zx - wy));
	result.setElem(9, 2.0f * (yz + wx));
	result.setElem(10, 1.0f - 2.0f * (x2 + y2));
	result.setElem(11, 0.);

	result.vec[3] = vec4f::from000W(1.);
	return result;
}

inline mat4f mat4f::transpose() const
{
	mat4f result;
	for (unsigned col = 0; col < 4; col++)
	{
		for (unsigned row = 0; row < 4; row++)
		{
			result.setElem(col * 4 + row, element()[col + row * 4]);
		}
	}
	return result;
}

inline mat3f mat3f::transpose() const
{
	mat3f result;
	for (unsigned col = 0; col < 3; col++)
	{
		for (unsigned row = 0; row < 3; row++)
		{
			result.setElem(col * 4 + row, element()[col + row * 4]);
		}
	}
	result.setElem(3, 0);
	result.setElem(7, 0);
	result.setElem(11, 0);
	return result;
}


#pragma pack()
