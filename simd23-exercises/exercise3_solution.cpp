/*
exercise3.cpp - Compute prefix sum with SIMD
Created: 2023-10-05
Author: hanna

*/

#include <stdint.h>
#include <immintrin.h>

EXERCISE_FN void e3(int *in, int *out, int count){
  __m256i zero = _mm256_set1_epi32(0);
  __m256i prev = zero;
#define X UINT32_MAX

  // TODO: For performance reasons we might want to consider _shuffle_epi8

  int i = 0;

  // NOTE(hanna): I tried unrolling this variant of the SIMD but it turns out to have the same performance.
  // Was hopping to issue more adds per cycle but doesn't seem like I can get more performance by doing this.
#if 0
  // 4x unrolled loop
  for(; i + 32 <= count; i += 32){
    __m256i v[4] = {
      _mm256_loadu_si256((__m256i*)(in + i + 0)),
      _mm256_loadu_si256((__m256i*)(in + i + 8)),
      _mm256_loadu_si256((__m256i*)(in + i + 16)),
      _mm256_loadu_si256((__m256i*)(in + i + 24))
    };

    {
      __m256i and_mask = _mm256_set_epi32(X, 0, X, 0,
                                          X, 0, X, 0);
      const int shuffle_mask = _MM_SHUFFLE(2, 0, 0, 0);

      v[0] = _mm256_add_epi32(v[0], _mm256_and_si256(_mm256_shuffle_epi32(v[0], shuffle_mask), and_mask));
      v[1] = _mm256_add_epi32(v[1], _mm256_and_si256(_mm256_shuffle_epi32(v[1], shuffle_mask), and_mask));
      v[2] = _mm256_add_epi32(v[2], _mm256_and_si256(_mm256_shuffle_epi32(v[2], shuffle_mask), and_mask));
      v[3] = _mm256_add_epi32(v[3], _mm256_and_si256(_mm256_shuffle_epi32(v[3], shuffle_mask), and_mask));
    }

    {
      __m256i and_mask = _mm256_set_epi32(X, X, 0, 0,
                                          X, X, 0, 0);
      const int shuffle_mask = _MM_SHUFFLE(1, 1, 0, 0);

      v[0] = _mm256_add_epi32(v[0], _mm256_and_si256(_mm256_shuffle_epi32(v[0], shuffle_mask), and_mask));
      v[1] = _mm256_add_epi32(v[1], _mm256_and_si256(_mm256_shuffle_epi32(v[1], shuffle_mask), and_mask));
      v[2] = _mm256_add_epi32(v[2], _mm256_and_si256(_mm256_shuffle_epi32(v[2], shuffle_mask), and_mask));
      v[3] = _mm256_add_epi32(v[3], _mm256_and_si256(_mm256_shuffle_epi32(v[3], shuffle_mask), and_mask));
    }

    {
      __m256i and_mask = _mm256_set_epi32(X, X, X, X,
                                          0, 0, 0, 0);
      const int shuffle_mask = _MM_SHUFFLE(3, 3, 3, 3);

      v[0] = _mm256_add_epi32(v[0], _mm256_and_si256(_mm256_broadcastd_epi32(_mm256_castsi256_si128(_mm256_shuffle_epi32(v[0], shuffle_mask))), and_mask));
      v[1] = _mm256_add_epi32(v[1], _mm256_and_si256(_mm256_broadcastd_epi32(_mm256_castsi256_si128(_mm256_shuffle_epi32(v[1], shuffle_mask))), and_mask));
      v[2] = _mm256_add_epi32(v[2], _mm256_and_si256(_mm256_broadcastd_epi32(_mm256_castsi256_si128(_mm256_shuffle_epi32(v[2], shuffle_mask))), and_mask));
      v[3] = _mm256_add_epi32(v[3], _mm256_and_si256(_mm256_broadcastd_epi32(_mm256_castsi256_si128(_mm256_shuffle_epi32(v[3], shuffle_mask))), and_mask));
    }

    v[0] = _mm256_add_epi32(v[0], prev);
    _mm256_storeu_si256((__m256i*)(out + i + 0), v[0]);

#define PREV(_v_) _mm256_broadcastd_epi32(_mm256_extractf128_si256(_mm256_shuffle_epi32((_v_), _MM_SHUFFLE(3, 3, 3, 3)), 1))

    v[1] = _mm256_add_epi32(v[1], PREV(v[0]));
    _mm256_storeu_si256((__m256i*)(out + i + 8), v[1]);

    v[2] = _mm256_add_epi32(v[2], PREV(v[1]));
    _mm256_storeu_si256((__m256i*)(out + i + 16), v[2]);

    v[3] = _mm256_add_epi32(v[3], PREV(v[2]));
    _mm256_storeu_si256((__m256i*)(out + i + 24), v[3]);

    prev = PREV(v[3]);
#undef PREV
  }
#endif

#if 0 // Original
  for(; i + 8 <= count; i += 8){
    __m256i v = _mm256_loadu_si256((__m256i*)(in + i));

    {
      __m256i and_mask = _mm256_set_epi32(X, 0, X, 0,
                                          X, 0, X, 0);
      __m256i v0 = _mm256_shuffle_epi32(v, _MM_SHUFFLE(2, 0, 0, 0));
      v0 = _mm256_and_si256(v0, and_mask);

      v = _mm256_add_epi32(v, v0);
    }

    {
      __m256i and_mask = _mm256_set_epi32(X, X, 0, 0,
                                          X, X, 0, 0);
      __m256i v0 = _mm256_shuffle_epi32(v, _MM_SHUFFLE(1, 1, 0, 0));
      v0 = _mm256_and_si256(v0, and_mask);

      v = _mm256_add_epi32(v, v0);
    }

    {
      __m256i and_mask = _mm256_set_epi32(X, X, X, X,
                                          0, 0, 0, 0);
      __m256i v0 = _mm256_shuffle_epi32(v, _MM_SHUFFLE(3, 3, 3, 3));
      v0 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(v0));
      v0 = _mm256_and_si256(v0, and_mask);

      v = _mm256_add_epi32(v, v0);
    }

    v = _mm256_add_epi32(v, prev);
    _mm256_storeu_si256((__m256i*)(out + i), v);

    {
      prev = _mm256_shuffle_epi32(v, _MM_SHUFFLE(3, 3, 3, 3));
      prev = _mm256_broadcastd_epi32(_mm256_extractf128_si256(prev, 1));
    }
  }
#else // Modified
  for(; i + 8 <= count; i += 8){
    __m256i v = _mm256_loadu_si256((__m256i*)(in + i));

#if 0
    {
      __m256i and_mask = _mm256_set_epi32(X, 0, X, 0,
                                          X, 0, X, 0);
      __m256i v0 = _mm256_shuffle_epi32(v, _MM_SHUFFLE(2, 0, 0, 0));
      v0 = _mm256_and_si256(v0, and_mask);

      v = _mm256_add_epi32(v, v0);
    }

#if 0 // NOTE(hanna): These variants seem to give very similar performance
    {
      __m256i and_mask = _mm256_set_epi32(X, X, 0, 0,
                                          X, X, 0, 0);
      __m256i v0 = _mm256_shuffle_epi32(v, _MM_SHUFFLE(1, 1, 0, 0));
      v0 = _mm256_and_si256(v0, and_mask);

      v = _mm256_add_epi32(v, v0);
    }
#else
    v = _mm256_add_epi32(v, _mm256_bslli_epi128(_mm256_shuffle_epi32(v, _MM_SHUFFLE(1, 1, 1, 1)), 8));
#endif
#else
    // NOTE(hanna): I cheated and stole some ideas from https://en.algorithmica.org/hpc/algorithms/prefix/
    // However it seems like the performance is very similar. But this is significantly more simple so lets keep it.
    v = _mm256_add_epi32(v, _mm256_slli_si256(v, 4));
    v = _mm256_add_epi32(v, _mm256_slli_si256(v, 8));
#endif

    {
      // NOTE(hanna): Doesn't seem to matter which variant we pick
#if 0
      __m256i and_mask = _mm256_set_epi32(X, X, X, X,
                                          0, 0, 0, 0);
      __m256i v0 = _mm256_shuffle_epi32(v, _MM_SHUFFLE(3, 3, 3, 3));
      v0 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(v0));
      v0 = _mm256_and_si256(v0, and_mask);
#else
      __m256i v0 = _mm256_shuffle_epi32(v, _MM_SHUFFLE(3, 3, 3, 3));
      v0 = _mm256_insertf128_si256(zero, _mm256_castsi256_si128(v0), 1);
#endif

      v = _mm256_add_epi32(v, v0);
    }

    v = _mm256_add_epi32(v, prev);
    _mm256_storeu_si256((__m256i*)(out + i), v);

    {
#if 0 // NOTE(hanna): The second variant here is significantly faster
      prev = _mm256_shuffle_epi32(v, _MM_SHUFFLE(3, 3, 3, 3));
      prev = _mm256_broadcastd_epi32(_mm256_extractf128_si256(prev, 1));
#else
      prev = (__m256i)_mm256_broadcast_ss((float*)(out + i + 7));
#endif
    }
  }
#endif

#undef X
}