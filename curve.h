#ifndef CURVE_H
#define CURVE_H

void bezier_casteljau(struct vector2 *res,
		      const struct vector2 *pts, int nr_pts, float t);

void bezier_bernstein(struct vector2 *res,
		      const struct vector2 *pts, int nr_pts, float t);

#endif
