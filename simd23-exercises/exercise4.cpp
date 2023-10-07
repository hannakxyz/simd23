/*
exercise4.cpp - Implement sorting using SIMD
Created: 2023-10-05

*/

#include <string.h>

EXERCISE_FN void e4(int *in, int *out, int count){

  for(int block_index = 0; block_index < count; block_index += 4, in += 4, out += 4){
    // NOTE(hanna): Insertion sort is fast for small number of items
    // An alternative would be to explicitly calculate the indices through comparisons.
    out[0] = in[0];
    for(int j = 1; j < 4; j += 1){
      int i = 0;
      for(; i < j; i += 1){
        if(out[i] > in[j]){
          break;
        }
      }
      memmove(out + i + 1, out + i, (j - i) * sizeof(int));
      out[i] = in[j];
    }
  }
}
