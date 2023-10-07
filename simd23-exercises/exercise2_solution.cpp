/*
exercise2_solution.cpp - Smallest element in a list
Created: 2023-10-05
Author: hanna

*/

static float e2(float *e, int count){
  __m256 min = _mm256_set1_ps(INFINITY);
  int cursor = 0;
  for(; cursor + 8 <= count; cursor += 8){
    __m256 v = _mm256_loadu_ps(&e[cursor]);
    min = _mm256_min_ps(min, v);
  }
  float min_e[8];
  _mm256_storeu_ps(min_e, min);
  float result = min_e[0];
  for(int i = 1; i < 8; i += 1){
    if(min_e[i] < result){
      result = min_e[i];
    }
  }
  for(; cursor < count; cursor += 1){
    if(e[cursor] < result){
      result = e[cursor];
    }
  }

  return result;
}
