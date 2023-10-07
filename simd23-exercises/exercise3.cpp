/*
exercise3.cpp - Compute prefix sum with SIMD
Created: 2023-10-05

*/

EXERCISE_FN void e3(int *e, int *out, int count){
  int sum = 0;
  int i = 0;
#if 1 // NOTE(hanna): Det är betydligt snabbare att använda en manuellt unrollad loop här!
  for(; i + 8 <= count; i += 8){
    sum += e[i + 0];
    out[i + 0] = sum;

    sum += e[i + 1];
    out[i + 1] = sum;

    sum += e[i + 2];
    out[i + 2] = sum;

    sum += e[i + 3];
    out[i + 3] = sum;

    sum += e[i + 4];
    out[i + 4] = sum;

    sum += e[i + 5];
    out[i + 5] = sum;

    sum += e[i + 6];
    out[i + 6] = sum;

    sum += e[i + 7];
    out[i + 7] = sum;
  }
#endif
#if 0
  for(; i < count; i += 1){
    sum += e[i];
    out[i] = sum;
  }
#endif
}