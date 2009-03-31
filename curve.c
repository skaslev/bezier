#include <alloca.h>
#include <string.h>

#include "vector2.h"
#include "util.h"
#include "curve.h"

void bezier(struct vector2 *res, const struct vector2 *pts, int nr_pts, float t)
{
	int i, j;
	struct vector2 *last, *curr;

	if (!nr_pts)
		return;

	last = alloca(sizeof(*last) * nr_pts);
	curr = alloca(sizeof(*curr) * nr_pts);
	memcpy(last, pts, sizeof(*last) * nr_pts);
	for (i = 0; i < nr_pts - 1; i++) {
		for (j = 0; j < nr_pts - i - 1; j++)
			vector2_lerp(&curr[j], &last[j], &last[j+1], t);
		SWAP(struct vector2 *, last, curr);
	}
	*res = last[0];
}

/* Caching is the last resort of ignorance. */
static int binomial(int n, int k)
{
#define MAXN		64
	static int cache[MAXN * (MAXN + 1) / 2 + MAXN];
	int idx;

	if (k < 0 || k > n)
		return 0;
	if (k == 0 || k == n)
		return 1;

	idx = n * (n + 1) / 2 + k;
	if (!cache[idx])
		cache[idx] = binomial(n - 1, k - 1) + binomial(n - 1, k);
	return cache[idx];
}

static float powi(float f, int i)
{
	float res;

	if (i < 0)
		return 1.0f / powi(f, -i);

	res = 1.0f;
	while (i)
		if (i % 2) {
			res *= f;
			i -= 1;
		} else {
			f *= f;
			i /= 2;
		}
	return res;
}

static inline float bernstein(int n, int i, float t)
{
	return binomial(n, i) * powi(t, i) * powi(1.0f - t, n - i);
}

void bezier_bernstein(struct vector2 *res, const struct vector2 *pts, int nr_pts, float t)
{
	int i;

	*res = (struct vector2) {};
	for (i = 0; i < nr_pts; i++) {
		struct vector2 pt = pts[i];
		vector2_mul(&pt, bernstein(nr_pts - 1, i, t));
		vector2_add(res, res, &pt);
	}
}
