#include <string.h>

#include "vector2.h"
#include "util.h"
#include "curve.h"

void bezier(struct vector2 *res, const struct vector2 *pts, int nr_pts, float t)
{
	int i, j;
	struct vector2 buf[nr_pts], buf2[nr_pts];
	struct vector2 *last = buf, *curr = buf2;

	memcpy(last, pts, sizeof(*last) * nr_pts);
	for (i = 0; i < nr_pts - 1; i++) {
		for (j = 0; j < nr_pts - i - 1; j++)
			vector2_lerp(&curr[j], &last[j], &last[j+1], t);
		SWAP(struct vector2 *, last, curr);
	}
	*res = last[0];
}
