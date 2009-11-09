#ifndef CURVE_H
#define CURVE_H

void bezier_casteljau(struct vec2 *res, const struct vec2 *pts, int nr_pts, float t);
void bezier_bernstein(struct vec2 *res, const struct vec2 *pts, int nr_pts, float t);

#endif
