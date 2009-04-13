#ifndef VECTOR2_H
#define VECTOR2_H

#include <math.h>

struct vector2 {
	float x, y;
};

/* Vector operations */
static inline void vector2_add(struct vector2 *r,
			       const struct vector2 *a, const struct vector2 *b)
{
	r->x = a->x + b->x;
	r->y = a->y + b->y;
}

static inline void vector2_sub(struct vector2 *r,
			       const struct vector2 *a, const struct vector2 *b)
{
	r->x = a->x - b->x;
	r->y = a->y - b->y;
}

static inline void vector2_mul(struct vector2 *r, float f)
{
	r->x *= f;
	r->y *= f;
}

static inline float vector2_dot(const struct vector2 *a,
				const struct vector2 *b)
{
	return a->x * b->x + a->y * b->y;
}

static inline float vector2_norm(const struct vector2 *a)
{
	return sqrt(vector2_dot(a, a));
}

static inline void vector2_normalize(struct vector2 *r)
{
	vector2_mul(r, 1.0f / vector2_norm(r));
}

static inline float vector2_dist(const struct vector2 *a,
				 const struct vector2 *b)
{
	struct vector2 d;
	vector2_sub(&d, a, b);
	return vector2_norm(&d);
}

static inline void vector2_lerp(struct vector2 *r,
				const struct vector2 *a, const struct vector2 *b, float t)
{
	r->x = t * a->x + (1.0f - t) * b->x;
	r->y = t * a->y + (1.0f - t) * b->y;
}

#endif
