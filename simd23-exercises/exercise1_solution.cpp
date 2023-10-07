/*
exercise1_solution.cpp -
Created: 2023-10-04
Author: hanna

*/

EXERCISE_FN void e1_scalar(float xa, float ya, float za,
                           float xb, float yb, float zb,
                           float &xc, float &yc, float &zc)
{
  xc = ya * zb - yb * za;
  yc = za * xb - xa * zb;
  zc = xa * yb - ya * xb;
}

EXERCISE_FN void e1_vector(__m128 xa, __m128 ya, __m128 za,
                           __m128 xb, __m128 yb, __m128 zb,
                           __m128 &xc, __m128 &yc, __m128 &zc)
{
  xc = _mm_sub_ps(_mm_mul_ps(ya, zb), _mm_mul_ps(yb, za));
  yc = _mm_sub_ps(_mm_mul_ps(za, xb), _mm_mul_ps(zb, xa));
  zc = _mm_sub_ps(_mm_mul_ps(xa, yb), _mm_mul_ps(xb, ya));
}
