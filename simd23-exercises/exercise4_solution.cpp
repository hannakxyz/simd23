/*
exercise4_solution.cpp - Implement sorting using SIMD
Created: 2023-10-05
Author: hanna

*/

#include <string.h>
#include <immintrin.h>
#include <stdint.h>

static void _sort_x4_step_ex(__m128i &t, __m128i t0, __m128i xor_value){
  __m128i cmp = _mm_cmpgt_epi32(t, t0);
  cmp = _mm_xor_si128(xor_value, cmp);
  t = _mm_blendv_epi8(t, t0, cmp);
}

EXERCISE_FN void e4(int *in, int *out, int count){
  for(int block_index = 0; block_index + 4 <= count; block_index += 4, in += 4, out += 4){
    __m128i v = _mm_loadu_si128((__m128i*)in);

#define _sort_x4_step(_t_, _shuffle_mask_, ...) \
  _sort_x4_step_ex((_t_), \
                   _mm_shuffle_epi32((_t_), (_shuffle_mask_)), \
                   _mm_set_epi32(__VA_ARGS__))

#define X UINT32_MAX

    // NOTE: Slarvigt ihopslängt sorteringsnätverk, kan göras mycket bättre!
    // Till exempel min/max-steg abs-steg kan vara mer effektiva

    _sort_x4_step(v, _MM_SHUFFLE(2, 3, 0, 1), X, 0, X, 0);
    _sort_x4_step(v, _MM_SHUFFLE(0, 1, 2, 3), X, X, 0, 0);
    _sort_x4_step(v, _MM_SHUFFLE(2, 3, 0, 1), X, 0, X, 0);

#undef X

    _mm_storeu_si128((__m128i*)out, v);
  }
}
