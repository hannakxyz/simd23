/*
exercise2.cpp - Smallest element in a list
Created: 2023-10-05

*/

static float e2(float *e, int count){
  // The exercise is to convert this to 8-wide SIMD somehow
  float result = INFINITY;
  for(int i = 0; i < count; i += 1){
    if(e[i] < result){
      result = e[i];
    }
  }
  return result;
}