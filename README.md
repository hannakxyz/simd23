# Hello there!

Här är några övningar till min lektion om SIMD. Resten följer på engelska.

To build and run the code on Linux just invoke ./build.sh

The point of these exercises is to get an introduction to using SIMD to then be able to explore more and use it more widely in algorithms.

## Exercise 1

In this exercise you write a simple routine for performing the cross product on 4 pairs of (geometric) vectors simultaneously. You will perhaps also be confused about the termonology as vector in this problem refers to two different things, both the SIMD registers and the mathematical vectors you are computing results on. But before doing any SIMD you will implement this in scalar code, i.e. code that only does one thing per instruction.
The code you write is then profiled (in a very naive manner) and checked for correctness.

```c
void e1_scalar(float xa, float ya, float za,
               float xb, float yb, float zb,
               float &xc, float &yc, float &zc);
```
The first part of this exercise is to implement the above routine. It takes as input a vector `(xa, ya, zb)` and another vector `(xb, yb, zb)` and spits out a third vector `(xc, yc, zc)`. The cross product satisfies
```
xc = ya * zb - yb * za
yc = za * xb - xa * zb
zc = xa * yb - ya * xb
```
when specified in these variables.

After implementing the scalar routine you are ready to tackle the SIMD variant. We will use 128 bit wide SIMD with 32 bit float elements. This means we have 128/32=4 floats in each SIMD register. The routine you are supposed to implement in this exercise is:
```c
void e1_vector(__m128 xa, __m128 ya, __m128 za,
               __m128 xb, __m128 yb, __m128 zb,
               __m128 &xc, __m128 &yc, __m128 &zc);
```
I would recommend looking up relevant instructions on the Intel Intrinsics Guide. To save time you could just implement it for the first component.

## Exercise 2

The next exercise is to write a routine for computing the minimum element of a list of given numbers.
Implement the following routine using 8-wide SIMD (you should use the __m256 type).
```c
float e2(float *e, int count);
```
The routine takes as input an array `e` with `count` number of elements and should return the minimum element in the array.

## Exercise 3

This exercise is about computing the prefix sum using SIMD.
You will implement a routine
```c
void e3(int *x, int *y, int count);
```
The routine takes as input one array `x` and writes its output to an `y`. Both arrays are of length `count`, which is guaranteed to be divisible by 8.
You will compute the prefix sum such that on return `y` satisfies
```c
y[0] = x[0]
y[1] = x[0] + x[1]
y[2] = x[0] + x[1] + x[2]
...
```
You will _not_ have to worry about integer overflows in this problem. You will also be working with arrays that fit in L1 cache so no need to worry about the prefetcher and such.

## Exercise 4

In this exercise you will sort positive integers <10^9 in batches of 4 numbers at a time, in ascending order. The reason for doing many batches at a time is to get more reasonable results from the profiling.
In the end each subarray consisting of elements at indices 0..3, 4..7, 8..13, ... will be sorted.
See the given scalar implementation or ask me if this is unclear.
This can be though of as a kernel for a full sorting algorithm which is invoked when working with a subproblem of <=4 elements.

```c
void e4(int *in, int *out, int count)
```

The passed `count` parameter will be divisible by 4.

The following image:
( https://en.wikipedia.org/wiki/Bitonic_sorter#/media/File:Batcher_Bitonic_Mergesort_for_eight_inputs.svg ) 
provides a hint about how to implement sorting. Sorting networks work by comparing and ordering 2 inputs at a time in a manner that can sometimes be parallelized. You can try implementing some other algorithm or sorting network if you like.

