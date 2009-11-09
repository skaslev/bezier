#ifndef VEC2_H
#define VEC2_H

#include <math.h>

struct vec2 {
	float x, y;
};

/* Vector operations */
#define vec2_set(r, xx, yy)	((r)->x = (xx),\
				 (r)->y = (yy))

#define vec2_add(r, a, b)	((r)->x = (a)->x + (b)->x,\
				 (r)->y = (a)->y + (b)->y)

#define vec2_sub(r, a, b)	((r)->x = (a)->x - (b)->x,\
				 (r)->y = (a)->y - (b)->y)

#define vec2_mul(r, f)		((r)->x *= (f),\
				 (r)->y *= (f))

#define vec2_mad(r, f, a)	((r)->x += (f) * (a)->x,\
				 (r)->y += (f) * (a)->y)

#define vec2_dot(a, b)		((a)->x * (b)->x + (a)->y * (b)->y)

#define vec2_norm(a)		(sqrtf(vec2_dot(a, a)))

#define vec2_normalize(r)	do { float n_ = 1.0f / vec2_norm(r);\
				     vec2_mul(r, n_); } while (0)

#define vec2_dist(a, b)		(sqrtf(((a)->x - (b)->x) * ((a)->x - (b)->x) +\
				       ((a)->y - (b)->y) * ((a)->y - (b)->y)))

#define vec2_lerp(r, a, b, t)	((r)->x = (t) * (a)->x + (1.0f - (t)) * (b)->x,\
				 (r)->y = (t) * (a)->y + (1.0f - (t)) * (b)->y)

#endif
