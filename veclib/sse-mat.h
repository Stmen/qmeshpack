#pragma once
#include "sse-quat.h"
#include "common-mat.h"

inline mat3f mat3f::from(const quatf q)
{
	mat3f result;
	__m128 v2 = q.vec * q.vec; //1
	__m128 xy_yz_zx = q.vec * sse::shuffle<1, 2, 0, 3>(q.vec); //2
	__m128 w_xyz = q.vec * sse::fromComponent<3>(q.vec); //3
	__m128 w_zxy = sse::shuffle<2, 0, 1, 3>(w_xyz); //4
	__m128 xpw = xy_yz_zx + w_zxy;
	__m128 xmw = xy_yz_zx - w_zxy;
	xpw += xpw; // 2*
	xmw += xmw; // 2*
	v2 += sse::shuffle<2, 0, 1, 3>(v2); //5 sum
	v2 += v2; // 2*

	__m128 ones = _mm_set1_ps(1.f);
	v2 = ones - v2; //6
	__v4si clear_w_mask =
	{ -1, -1, -1, 0 };

	result.vec[0].vec = sse::shuffle<0, 0, 2, 2>(xmw, xpw);
	result.vec[0].vec = _mm_move_ss(result.vec[0].vec,
			sse::fromComponent<2>(v2));
	result.vec[0].vec = _mm_and_ps(result.vec[0].vec, (__m128 ) clear_w_mask);

	result.vec[1].vec = sse::shuffle<0, 0, 1, 1>(v2, xmw);
	result.vec[1].vec = _mm_move_ss(result.vec[1].vec,
			sse::fromComponent<0>(xpw));
	result.vec[1].vec = _mm_and_ps(result.vec[1].vec, (__m128 ) clear_w_mask);

	result.vec[2].vec = sse::shuffle<1, 1, 1, 1>(xpw, v2);
	result.vec[2].vec = _mm_move_ss(result.vec[2].vec,
			sse::fromComponent<2>(xmw));
	result.vec[2].vec = _mm_and_ps(result.vec[2].vec, (__m128 ) clear_w_mask);

	return result;
}

inline mat4f mat4f::from(const quatf q)
{
	mat4f result;
	__m128 v2		= q.vec * q.vec; //1
	__m128 xy_yz_zx = q.vec * sse::shuffle<1, 2, 0, 3>(q.vec); //2
	__m128 w_xyz	= q.vec * sse::fromComponent<3>(q.vec); //3
	__m128 w_zxy	= sse::shuffle<2, 0, 1, 3>(w_xyz); //4
	__m128 xpw		= xy_yz_zx + w_zxy;
	__m128 xmw		= xy_yz_zx - w_zxy;
	xpw += xpw; // 2*
	xmw += xmw; // 2*
	v2 += sse::shuffle<2, 0, 1, 3>(v2); //5 sum
	v2 += v2; // 2*

	__m128 ones = _mm_set1_ps(1.f);
	v2 = ones - v2; //6
	__v4si clear_w_mask = {-1, -1, -1, 0};

	result.vec[0].vec = sse::shuffle<0, 0, 2, 2>(xmw, xpw);
	result.vec[0].vec = _mm_move_ss(result.vec[0].vec, sse::fromComponent<2>(v2));
	result.vec[0].vec = _mm_and_ps(result.vec[0].vec, (__m128)clear_w_mask);

	result.vec[1].vec = sse::shuffle<0, 0, 1, 1>(v2, xmw);
	result.vec[1].vec = _mm_move_ss(result.vec[1].vec, sse::fromComponent<0>(xpw));
	result.vec[1].vec = _mm_and_ps(result.vec[1].vec, (__m128)clear_w_mask);

	result.vec[2].vec = sse::shuffle<1, 1, 1, 1>(xpw, v2);
	result.vec[2].vec = _mm_move_ss(result.vec[2].vec, sse::fromComponent<2>(xmw));
	result.vec[2].vec = _mm_and_ps(result.vec[2].vec, (__m128)clear_w_mask);

	result.vec[3] = vec4f::from000W(1.f);
	return result;
}

inline  mat3f mat3f::transpose() const
{
	mat3f result;
	__m128 dummy = vec4f::from000W(1.).vec;
	__m128 t0 = _mm_unpacklo_ps(vec[0].vec, vec[1].vec);
	__m128 t1 = _mm_unpacklo_ps(vec[2].vec, dummy);
	__m128 t2 = _mm_unpackhi_ps(vec[0].vec, vec[1].vec);
	__m128 t3 = _mm_unpackhi_ps(vec[2].vec, dummy);
	result.vec[0].vec = _mm_movelh_ps(t0, t1);
	result.vec[1].vec = _mm_movehl_ps(t1, t0);
	result.vec[2].vec = _mm_movelh_ps(t2, t3);
	return result;
}

inline  mat4f mat4f::transpose() const
{
	mat4f result;
	__m128 t0 = _mm_unpacklo_ps(vec[0].vec, vec[1].vec);
	__m128 t1 = _mm_unpacklo_ps(vec[2].vec, vec[3].vec);
	__m128 t2 = _mm_unpackhi_ps(vec[0].vec, vec[1].vec);
	__m128 t3 = _mm_unpackhi_ps(vec[2].vec, vec[3].vec);
	result.vec[0].vec = _mm_movelh_ps(t0, t1);
	result.vec[1].vec = _mm_movehl_ps(t1, t0);
	result.vec[2].vec = _mm_movelh_ps(t2, t3);
	result.vec[3].vec = _mm_movehl_ps(t3, t2);
	return result;
}

#pragma pack(16)

