/*
hanna_util.h - Utility code for most of my projects in C!

Created: 2020-11-27
Author: hanna
License: Public Domain. See bottom of this file.

         The PCG PRNG code included here has license:
         *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
         Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

Essentially a single header library but all procedures are defined as static;
there is no HANNA_UTIL_IMPLEMENTATION macro as in most single header libraries.

TODOS:

Make there be a global out of memory handler?

// PUSHING HUGE AMOUNTS OF MEMORY SHOULD BE HANDLED WITHOUT THROWING AWAY THE CURRENT CHUNK IN THE ALLOCATOR

IMPLEMENT PROPER FLOAT TO STRING CONVERSION WITH SEGMENTED RYU

MOVE EVERYTHING OVER INTO THE NEW PRINTF IMPLEMENTATION

TODO: we should free all temporary allocators when a oom situation is reached. We should store some kind of list of temp allocators here!
Also consider detecting mutex locks wrt to oom situations.

MAKE A DEBUGGER TOOL FOR SHOWING STATE OF MEMORY ALLOCATORS GRAPHICALLY

MAKE THE THREAD POOL USE A CONDITION VARIABLE INSTEAD OF A SEMAPHORE!

WE SHOULD REALLY MAKE ASSERT HAVE A DIFFERENT NAME, SO IT DOESNT CLASH WITH THE BUILTIN ASSERT, E.G. USE ASSERT UPPERCASED

SHOULD TIMESTAMPS USE I64 INSTEAD OF U64?

MAKE USE OF THE ERRORS INTERFACE IN SYSCALLS

*/

#ifndef HANNA_UTIL_H
#define HANNA_UTIL_H

//~ Detect architecture

#define ARCH_X86_64  0
#define ARCH_ARM64   0

#if defined(__x86_64__) || defined(_M_AMD64)
#  undef ARCH_X86_64
#  define ARCH_X86_64 1
#elif defined(__arm__)
#  error "We don't support 32 bit ARM!"
#elif defined(__aarch64__)
#  undef ARCH_ARM64
#  define ARCH_ARM64 1
#else
#  error "Unable to determine architecture!"
#endif

#if ARCH_X86_64
#  define OS_PAGE_SIZE LIT_U64(4096)
#  define OS_PAGE_SIZE_LOG2 LIT_U64(12)
#elif ARCH_ARM64
#  define OS_PAGE_SIZE LIT_U64(4096)
#  define OS_PAGE_SIZE_LOG2 LIT_U64(12)
#else
#  error "Unknown architecture"
#endif

//~ Detect operating system

#define OS_LINUX   0
#define OS_WINDOWS 0

#if defined(__linux__)
#  undef OS_LINUX
#  define OS_LINUX 1
#elif defined(_WIN32)
#  undef OS_WINDOWS
#  define OS_WINDOWS 1
#else
#  error "Unable to determine operating system!"
#endif

//~ Detect compiler

#define COMPILER_MSVC 0
#define COMPILER_GCC 0

#if defined(_MSC_VER)
#  undef COMPILER_MSVC
#  define COMPILER_MSVC 1
#elif defined(__GNUC__)
#  undef COMPILER_GCC
#  define COMPILER_GCC 1
#else
#  error "Unknown compiler!"
#endif

//~ Includes

#if OS_LINUX
#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <sys/random.h>
#elif OS_WINDOWS
#include <windows.h>
#else
#error "Unknown architecture"
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_GCC
#include <x86intrin.h>
#else
#error "Unknown compiler"
#endif

#if ARCH_X86_64
#include <immintrin.h>
#include <pmmintrin.h>
#include <emmintrin.h>
#elif ARCH_ARM64
// nothing here yet...
#else
#error "Unknown architecture"
#endif

// TODO(hanna - 2021-02-15): Don't use the standard library!
#include <math.h>
#include <setjmp.h>

//~ STB SPRINTF

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#include "3rdparty/stb_sprintf.h"

//~ Types

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

//~ Sane names for data types
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t  U8;

typedef int64_t I64;
typedef int32_t I32;
typedef int16_t I16;
typedef int8_t  I8;

typedef float  F32;
typedef double F64;

/* Intended for when you need your enum value to be stored as a specific size, but still want to mark up what enum kind it is. E.g.
Enum32(TokenKind) kind;
 */
#define EnumU8(_Name_) U8
#define EnumU16(_Name_) U16
#define EnumU32(_Name_) U32

//~ Literals

#define LIT_U64(_value_) ( _value_ ## ULL )
#define LIT_I64(_value_) ( _value_ ## LL )
#define LIT_U32(_value_) ( _value_ ## U )
#define LIT_I32(_value_) ( _value_ )

//~ Sane #defines

#define _CT_ASSERT_NAME2(_counter_) compile_time_assert##_counter_
#define _CT_ASSERT_NAME(_counter_) _CT_ASSERT_NAME2(_counter_)
#define CT_ASSERT(_condition_) \
  typedef struct{ int x[(_condition_) ? 1 : -1]; } _CT_ASSERT_NAME(__COUNTER__)

// NOTE(hanna): Seems like (newer versions?) of MSVC complains that _STRINGIZE is already defined, so for now this is named _STRINGIZE2
#define _STRINGIZE2(_a_) #_a_
#define STRINGIZE(_a_) _STRINGIZE2(_a_)

#define _COMBINE2(_a_, _b_) _a_##_b_
#define COMBINE2(_a_, _b_) _COMBINE2(_a_, _b_)
#define COMBINE3(_a_, _b_, _c_) COMBINE2(_a_, COMBINE2(_b_, _c_))
#define COMBINE4(_a_, _b_, _c_, _d_) COMBINE2(_a_, COMBINE3(_b_, _c_, _d_))

#define IS_POWER_OF_TWO(_value_) ((((_value_) - 1) & (_value_)) == 0)

#define SIGN_OF_(_x_) ( ((_x_) < 0) ? (-1) : ( ((_x_) > 0) ? (1) : (0) ) )
#define ABSOLUTE_VALUE(_x_) ( ((_x_) < 0) ? (-(_x_)) : (((_x_) == 0) ? 0 : (_x_)) )
#define CLAMP(_a_, _x_, _b_) ( (_x_) < (_a_) ? (_a_) : ( (_x_) < (_b_) ? (_x_) : (_b_) ) )
#define CLAMP01(_x_) CLAMP(0, (_x_), 1)

// NOTE(hanna): These macros are copied from Shawn McGrath from when he used to stream. Thanks!
#define fiz(_count_) for(I64 i = 0; i < (_count_); i += 1)
#define fjz(_count_) for(I64 j = 0; j < (_count_); j += 1)
#define fkz(_count_) for(I64 k = 0; k < (_count_); k += 1)

#define fiz_U32(_count_) for(U32 i = 0; i < (_count_); i += 1)
#define fjz_U32(_count_) for(U32 j = 0; j < (_count_); j += 1)
#define fkz_U32(_count_) for(U32 k = 0; k < (_count_); k += 1)

#define ARRAY_COUNT(_array_) (sizeof(_array_) / sizeof((_array_)[0]))

#define MATH_PI (3.141592653589793238462643383279502884)
#define MATH_E (2.7182818284590452353602874713526624977572470936)
#define MATH_TAU (MATH_PI * 2.0)

#define DEG_TO_RAD(_value_) ((_value_) * (MATH_TAU / 360.0))
#define RAD_TO_DEG(_value_) ((_value_) * (360.0 / MATH_TAU))
#define SQUARE(_value_) ((_value_) * (_value_))

#define MINIMUM(_a_, _b_) ((_a_) < (_b_) ? (_a_) : (_b_))
#define MINIMUM3(_a_, _b_, _c_) ( MINIMUM((_a_), MINIMUM((_b_), (_c_))) )

#define MAXIMUM(_a_, _b_) ((_a_) > (_b_) ? (_a_) : (_b_))
#define MAXIMUM3(_a_, _b_, _c_) (MAXIMUM((_a_), MAXIMUM((_b_), (_c_))))

// Intended usage is IN_RANGE(lower <=, middle, <= upper) to check if lower <= middle <= upper
#define IN_RANGE(_a_, _b_, _c_) (_a_ _b_ && _b_ _c_)

#define KILOBYTES(_value_) ((_value_) * LIT_U64(1024))
#define MEGABYTES(_value_) ((_value_) * LIT_U64(1024) * LIT_U64(1024))
#define GIGABYTES(_value_) ((_value_) * LIT_U64(1024) * LIT_U64(1024) * LIT_U64(1024))

// ALIGNOF macro
// NOTE(hanna): The alignof situation is that MSVC does not support alignof(expression), only alignof(type) but GCC does support both

#if COMPILER_MSVC
#define ALIGNOF_EXPR(...) 64 // Maximum align required by anyone
#define ALIGNOF_TYPE(...) ( __alignof(__VA_ARGS__) )
#elif COMPILER_GCC
#define ALIGNOF_EXPR(...) ( __alignof__(__VA_ARGS__) )
#define ALIGNOF_TYPE(...) ( __alignof__(__VA_ARGS__) )
#else
#error "Unknown compiler"
#endif

//~ Logging, panic and assertions.

#if ARCH_X86_64 && OS_LINUX
static void debug_trap(){
  __asm__("int3");
}
#elif COMPILER_MSVC
#include <windows.h>
static void debug_trap() {
  __debugbreak();
}
#elif ARCH_ARM64 && OS_LINUX
#include <unistd.h>
#include <signal.h>
static void debug_trap(){
  raise(SIGTRAP);
}
#else
#  error "Don't know how to do debug_trap on this compiler/OS/Architecture combination"
#endif

#if COMPILER_GCC || COMPILER_CLANG
__attribute__((__noreturn__))
#endif
static void _panic(const char *file, int line, const char *procedure_signature, const char *format, ...){
  va_list list;
  va_start(list, format);

  char buffer[4096];
  stbsp_vsnprintf(buffer, sizeof(buffer), format, list);
  fprintf(stderr, "PANIC on %s:%d (%s)! FATAL ERROR: %s\n", file, line, procedure_signature, buffer);

  va_end(list);

  debug_trap();
  exit(1);
}

#if COMPILER_GCC
#define panic(...) _panic(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#elif COMPILER_MSVC
#define panic(...) _panic(__FILE__, __LINE__, __FUNCSIG__, __VA_ARGS__)
#else
#error "Unknown compiler"
#endif

//~ Assert

// NOTE(hanna): Because assert.h is very aggresive and #undefs assert and we don't want that to happen we detect assert being included
// By declaring __assert_fail with a different signature from the one used in assert.h. This should hopefully allow us to detect assert.h
// being included.
#if __cplusplus
extern "C"
#endif
void __assert_fail (void *garbage);

#if !defined(HANNA_ENABLE_ASSERTS)
#error "HANNA_ENABLE_ASSERTS is not defined! You must #define HANNA_ENABLE_ASSERTS to either 1 or 0, depending on whether you want asserts or not be enabled."
#endif

#if HANNA_ENABLE_ASSERTS != 0 && HANNA_ENABLE_ASSERTS != 1
#error "HANNA_ENABLE_ASSERTS is not defined to 0 or 1. You must #define HANNA_ENABLE_ASSERTS to either 1 or 0, depending on whether you want asserts or not be enabled."
#endif

#if HANNA_ENABLE_ASSERTS
static int assertion_failed(const char *condition, int line, const char *file){
  fprintf(stderr, "(%s:%d) Assertion %s failed\n", file, line, condition);
  debug_trap();
  return 0;
}

#undef assert
#define assert(_condition_) ((_condition_) ? 0 : assertion_failed(#_condition_, __LINE__, __FILE__))
#else
#undef assert
#define assert(_condition_) ((void)(_condition_))
#endif

//~ Basic memory operations

#if 0
static void copy_memory(void *dst, size_t dst_size,
                        void *src, size_t src_size)
{
  assert(dst_size == src_size);
  memcpy(dst, src, dst_size);
}
#endif

#define copy_items(_dst_, _src_, _Type_, _count_) _copy_items((_dst_), (_src_), sizeof(_Type_), (_count_))
static void _copy_items(void *dst, void *src, size_t element_size, size_t count){
  assert(element_size < UINT32_MAX);
  assert(count < UINT32_MAX);
  size_t size = element_size * count;
  // TODO: Just detect the overflow and panic on that!
  memmove(dst, src, size);
}

#define clear_items(_dst_, _Type_, _count_) _clear_items((_dst_), sizeof(_Type_), (_count_))
static void _clear_items(void *dst, size_t element_size, size_t count){
  assert(element_size < UINT32_MAX);
  assert(count < UINT32_MAX);
  // TODO: Just detect the overflow and panic on that!
  size_t size = element_size * count;
  memset(dst, 0, size);
}

#define clear_memory(_x_, _size_) memset((_x_), 0, (_size_))
#define clear_item(_x_) clear_memory((_x_), sizeof(*(_x_)))

static bool memory_equals(void *a, size_t size_a, void *b, size_t size_b){
  return (size_a == size_b) && (memcmp(a, b, size_a) == 0);
}

//~ Basic intrinsics

#if COMPILER_GCC
static U32 index_of_low_bit_u32(U32 value){
  U32 result;
  if(value == 0){
    result = 32;
  }else{
    result = __builtin_ctz(value);
  }
  return result;
}
static U64 index_of_low_bit_u64(U64 value){
  U64 result;
  if(value == 0){
    result = 64;
  }else{
    result = __builtin_ctzll(value);
  }
  return result;
}
static U32 index_of_high_bit_u32(U32 value){
  U32 result;
  if(value == 0){
    result = 32;
  }else{
    result = 31 - __builtin_clz(value);
  }
  return result;
}
static U64 index_of_high_bit_u64(U64 value){
  U64 result;
  if(value == 0){
    result = 64;
  }else{
    result = 63 - __builtin_clzll(value);
  }
  return result;
}

static U64 count_leading_zeros_u64(U64 value){
  return __builtin_clzl(value);
}

#elif COMPILER_MSVC
static U32 index_of_low_bit_u32(U32 value){
  U32 result;
  if(!BitScanForward((unsigned long*)&result, value)){
    result = 32;
  }
  return result;
}
static U64 index_of_low_bit_u64(U64 value){
  U32 result;
  if(!BitScanForward64((unsigned long*)&result, value)){
    result = 64;
  }
  return result;
}
static U32 index_of_high_bit_u32(U32 value){
  U32 result;
  if(!BitScanReverse((unsigned long*)&result, value)){
    result = 32;
  }
  return result;
}
static U64 index_of_high_bit_u64(U64 value){
  U32 result;
  if(!BitScanReverse64((unsigned long*)&result, value)){
    result = 64;
  }
  return (U64)result;
}

static U64 count_leading_zeros_u64(U64 value){
  return __lzcnt64(value);
}

#else
#error "Unknown compiler"
#endif

// ======================
// Basic OS interaction
// ======================

//~ OS-provided mutex

// NOTE(hanna): You may not move an initialized mutex!!
#if OS_LINUX
typedef struct OSMutex{ pthread_mutex_t value; } OSMutex;
#elif OS_WINDOWS
#include <windows.h>
typedef struct OSMutex{ CRITICAL_SECTION critical_section; } OSMutex;
#else
#error "Unknown OS"
#endif
static void os_mutex_init(OSMutex *mutex);
static void os_mutex_destroy(OSMutex *mutex);
static void os_mutex_lock(OSMutex *mutex);
static void os_mutex_unlock(OSMutex *mutex);

//~ Semaphore

#if OS_LINUX
#include <semaphore.h>
typedef struct Semaphore{ sem_t value; } Semaphore;
#elif OS_WINDOWS
#include <windows.h>
typedef struct Semphore{ HANDLE handle; } Semaphore;
#else
#error "Unknown OS"
#endif
static void semaphore_init(Semaphore *sem);
static void semaphore_destroy(Semaphore *sem);
static void semaphore_post(Semaphore *sem);
static void semaphore_wait(Semaphore *sem);
//static bool semaphore_trywait(Semaphore *sem);
//static bool semaphore_timedwait_ns(Semaphore *sem, U64 duration);

//~ Condition variable

#if OS_LINUX
typedef struct OSCondVar{ pthread_cond_t value; } OSCondVar;
#elif OS_WINDOWS
typedef struct OSCondVar{ CONDITION_VARIABLE value; } OSCondVar;
#else
#error "Unknown OS"
#endif

static void os_cond_var_init(OSCondVar *cond);
static void os_cond_var_destroy(OSCondVar *cond);
static void os_cond_var_wait(OSCondVar *cond, OSMutex *mutex);
static void os_cond_var_wake_one(OSCondVar *cond);
static void os_cond_var_wake_all(OSCondVar *cond);

//~ Implementation

#if OS_LINUX

//~ OSMutex

// TODO: We should make some proper comparison with using a Handmade Hero-style ticket mutex (perhaps use ips-student-challenge as a benchmark)

static void os_mutex_init(OSMutex *mutex){
  if(pthread_mutex_init(&mutex->value, NULL) != 0){
    panic("pthread_mutex_init() returned non-zero: errno = %d", errno);
  }
}
static void os_mutex_destroy(OSMutex *mutex){
  if(pthread_mutex_destroy(&mutex->value) != 0){
    panic("pthreaod_mutex_destroy() returned non-zero: errno = %d", errno);
  }
}
static void os_mutex_lock(OSMutex *mutex){
  if(pthread_mutex_lock(&mutex->value) != 0){
    panic("pthread_mutex_lock() returned non-zero: errno = %d", errno);
  }
}
static void os_mutex_unlock(OSMutex *mutex){
  if(pthread_mutex_unlock(&mutex->value) != 0){
    panic("pthread_mutex_unlock() returned non-zero: errno = %d", errno);
  }
}

// @TODO @IMPORTANT: Handle EINTR here
static void semaphore_init(Semaphore *sem){
  if(sem_init(&sem->value, 0, 0) == -1){ // does not fail with EINTR
    panic("sem_init() returned -1: errno = %d", errno);
  }
}
static void semaphore_destroy(Semaphore *sem){
  if(sem_destroy(&sem->value) == -1){ // does not fail with EINTR
    panic("sem_destroy() returned -1: errno = %d", errno);
  }
}
static void semaphore_post(Semaphore *sem){
  if(sem_post(&sem->value) == -1){ // does not fail with EINTR
    panic("sem_post() returned -1: errno = %d", errno);
  }
}
static void semaphore_wait(Semaphore *sem){
  int status;
  while((status = sem_wait(&sem->value)) == -1 && errno == EINTR);
  if(status == -1){
    panic("sem_wait() returned -1: errno = %d", errno);
  }
}
static bool semaphore_trywait(Semaphore *sem){
  bool result = true;
  int status;
  while((status = sem_trywait(&sem->value)) == -1 && errno == EINTR);
  int errno_value = errno;
  if(status == -1){
    if(errno_value == EAGAIN){
      result = false;
    }else{
      panic("sem_trywait() returned -1: errno = %d", errno_value);
    }
  }
  return result;
}
static bool semaphore_timedwait_ns(Semaphore *sem, U64 duration){
  bool result = true;
  // TODO: Verify that this actually works!!
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  U64 absolute_timeout = (uint64_t)ts.tv_sec * LIT_U64(1000000000) + (uint64_t)ts.tv_nsec;
  absolute_timeout += duration;
  struct timespec sleep_time = (struct timespec){
    .tv_sec = (I64)(absolute_timeout / LIT_U64(1000000000)),
    .tv_nsec = (I64)(absolute_timeout % LIT_U64(1000000000))
  };
  int status;
  while((status = sem_timedwait(&sem->value, &sleep_time)) == -1 && errno == EINTR);
  int errno_value = errno;
  if(status == -1){
    if(errno_value == ETIMEDOUT || errno_value == EAGAIN){
      result = false;
    }else{
      panic("sem_timedwait() returned -1: errno = %d", errno_value);
    }
  }
  return result;
}
static int semaphore_get_value(Semaphore *sem){
  int result;
  if(sem_getvalue(&sem->value, &result) == -1){
    panic("sem_getvalue() returned -1: errno = %d", errno);
  }
  if(result < 0){
    panic("sem_getvalue() gave back a negative semaphore value. We don't support systems with this behaviour. (But we trivially could by clamping the value.)");
  }
  return result;
}

static void os_cond_var_init(OSCondVar *cond){
  if(pthread_cond_init(&cond->value, NULL) != 0){
    panic("pthread_cond_init() failed: errno=%d: %s", errno, strerror(errno));
  }
}
static void os_cond_var_destroy(OSCondVar *cond){
  if(pthread_cond_destroy(&cond->value) != 0){
    panic("pthread_cond_destroy() failed: errno=%d: %s", errno, strerror(errno));
  }
}
static void os_cond_var_wait(OSCondVar *cond, OSMutex *mutex){
  int error = pthread_cond_wait(&cond->value, &mutex->value);
  if(error != 0){
    panic("pthread_cond_wait() failed: errno=%d: %s", error, strerror(error));
  }
}
static void os_cond_var_wake_one(OSCondVar *cond){
  int error = pthread_cond_signal(&cond->value);
  if(error != 0){
    panic("pthread_cond_signal() failed: errno=%d: %s", error, strerror(error));
  }
}
static void os_cond_var_wake_all(OSCondVar *cond){
  int error = pthread_cond_broadcast(&cond->value);
  if(error != 0){
    panic("pthread_cond_broadcast() failed: errno=%d: %s", error, strerror(error));
  }
}

#elif OS_WINDOWS

static void os_mutex_init(OSMutex *mutex){
  InitializeCriticalSection(&mutex->critical_section);
}
static void os_mutex_destroy(OSMutex *mutex){
  DeleteCriticalSection(&mutex->critical_section);
}
static void os_mutex_lock(OSMutex *mutex){
  EnterCriticalSection(&mutex->critical_section);
}
static void os_mutex_unlock(OSMutex *mutex){
  LeaveCriticalSection(&mutex->critical_section);
}

static void semaphore_init(Semaphore *sem){
  sem->handle = CreateSemaphore(NULL, 0, UINT32_MAX, NULL);
  if(!sem->handle){
    panic("CreateSemaphore failed: GetLastError: %u", GetLastError());
  }
}
static void semaphore_destroy(Semaphore *sem){
  if(!CloseHandle(&sem->handle)){
    panic("CloseHandle failed!");
  }
}
static void semaphore_post(Semaphore *sem){
  if(!ReleaseSemaphore(sem->handle, 1, NULL)){
    panic("ReleaseSemaphore failed");
  }
}
static void semaphore_wait(Semaphore *sem){
  DWORD status = WaitForSingleObject(sem->handle, INFINITE);
  if(status == WAIT_FAILED){
    panic("WaitForSingleObject failed: GetLastError: %u", GetLastError());
  }else if(status == WAIT_OBJECT_0){
    // Success!!
  }else{
    panic("WaitForSingleObject didnt return WAIT_OBJECT_0");
  }
}
static bool semaphore_trywait(Semaphore *sem){
  panic("This routine is not implemented in Windows yet!");
}
static bool semaphore_timedwait_ns(Semaphore *sem, U64 duration){
  panic("This routine is not implemented in Windows yet!");
}

static int semaphore_get_value(Semaphore *sem){
  panic("This routine is not implemented in Windows yet!");
}


static void os_cond_var_init(OSCondVar *cond){
  InitializeConditionVariable(&cond->value);
}

static void os_cond_var_destroy(OSCondVar *cond){
  // Nothing to do, Win32 is nice and doesn't care
}
static void os_cond_var_wait(OSCondVar *cond, OSMutex *mutex){
  if(SleepConditionVariableCS(&cond->value, &mutex->critical_section, INFINITE) == 0){
    int error = GetLastError();
    panic("SleepConditionVariableCS failed: GetLastError() -> %d", error);
  }
}
static void os_cond_var_wake_one(OSCondVar *cond){
  WakeConditionVariable(&cond->value);
}
static void os_cond_var_wake_all(OSCondVar *cond){
  WakeAllConditionVariable(&cond->value);
}
#else
#error "Unknown OS"
#endif

// ==================
// GENERAL UTILITY
// ==================

//~ Atomics

#if COMPILER_GCC
// https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
// https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html

// NOTE: Atomics may not be placed across cache lines!!
typedef struct __attribute__((aligned(4))) AtomicU32{ U32 _value; } AtomicU32;
static U32 atomic_read_u32(AtomicU32 *atomic){ return __atomic_load_4(&atomic->_value, __ATOMIC_SEQ_CST); }
static void atomic_store_u32(AtomicU32 *atomic, U32 value){ __atomic_store_4(&atomic->_value, value, __ATOMIC_SEQ_CST); }
static U32 atomic_exchange_u32(AtomicU32 *atomic, U32 value){ return __atomic_exchange_4(&atomic->_value, value, __ATOMIC_SEQ_CST); }
static U32 atomic_add_u32(AtomicU32 *atomic, U32 value){ return __atomic_fetch_add(&atomic->_value, value, __ATOMIC_SEQ_CST); }
static U32 atomic_sub_u32(AtomicU32 *atomic, U32 value){ return __atomic_fetch_sub(&atomic->_value, value, __ATOMIC_SEQ_CST); }
static bool atomic_compare_exchange_u32(AtomicU32 *atomic, U32 old_value, U32 new_value){
  return __atomic_compare_exchange_4(&atomic->_value,
                                     &old_value,
                                     new_value,
                                     /*weak=*/false,
                                     /*success memorder*/__ATOMIC_SEQ_CST,
                                     /*failure memorder*/__ATOMIC_SEQ_CST);
}

// NOTE: Atomics may not be placed across cache lines!!
typedef struct __attribute__((aligned(8))) AtomicU64{ U64 _value; } AtomicU64;
static U64 atomic_read_u64(AtomicU64 *atomic){ return __atomic_load_8(&atomic->_value, __ATOMIC_SEQ_CST); }
static void atomic_store_u64(AtomicU64 *atomic, U64 value){ __atomic_store_8(&atomic->_value, value, __ATOMIC_SEQ_CST); }
static U64 atomic_exchange_u64(AtomicU64 *atomic, U64 value){ return __atomic_exchange_8(&atomic->_value, value, __ATOMIC_SEQ_CST); }
static U64 atomic_add_u64(AtomicU64 *atomic, U64 value){ return __atomic_fetch_add(&atomic->_value, value, __ATOMIC_SEQ_CST); }
static U64 atomic_sub_u64(AtomicU64 *atomic, U64 value){ return __atomic_fetch_sub(&atomic->_value, value, __ATOMIC_SEQ_CST); }
static bool atomic_compare_exchange_u64(AtomicU64 *atomic, U64 old_value, U64 new_value){
  return __atomic_compare_exchange_8(&atomic->_value,
                                     &old_value,
                                     new_value,
                                     /*weak=*/false,
                                     /*success memorder*/__ATOMIC_SEQ_CST,
                                     /*failure memorder*/__ATOMIC_SEQ_CST);
}
#elif COMPILER_MSVC

// TODO(hanna): We should really make sure that we did this correctly!

// NOTE: Atomics may not be placed across cache lines!!
typedef struct __declspec(align(4)) AtomicU32{ volatile U32 _value; } AtomicU32;
static U32 atomic_read_u32(AtomicU32 *atomic){ return atomic->_value; }
static void atomic_store_u32(AtomicU32 *atomic, U32 value){ InterlockedExchange(&atomic->_value, value); }
static U32 atomic_exchange_u32(AtomicU32 *atomic, U32 value){ return InterlockedExchange(&atomic->_value, value); }
static U32 atomic_add_u32(AtomicU32 *atomic, U32 value){ return InterlockedAdd((volatile LONG*)&atomic->_value, value); }
static U32 atomic_sub_u32(AtomicU32 *atomic, U32 value){ return InterlockedAdd((volatile LONG*)&atomic->_value, 0-value); }
static bool atomic_compare_exchange_u32(AtomicU32 *atomic, U32 old_value, U32 new_value){
  LONG init_value = InterlockedCompareExchange(&atomic->_value, new_value, old_value);
  return init_value == old_value;
}

// NOTE: Atomics may not be placed across cache lines!!
typedef struct __declspec(align(8)) AtomicU64{ volatile U64 _value; } AtomicU64;
static U64 atomic_read_u64(AtomicU64 *atomic){ return atomic->_value; }
static void atomic_store_u64(AtomicU64 *atomic, U64 value){ InterlockedExchange64((volatile LONG64*)&atomic->_value, value); }
static U64 atomic_exchange_u64(AtomicU64 *atomic, U64 value){ return InterlockedExchange64((volatile LONG64*)&atomic->_value, value); }
static U64 atomic_add_u64(AtomicU64 *atomic, U64 value){ return InterlockedAdd64((volatile LONG64*)&atomic->_value, value); }
static U64 atomic_sub_u64(AtomicU64 *atomic, U64 value){ return InterlockedAdd64((volatile LONG64*)&atomic->_value, 0-value); }
static bool atomic_compare_exchange_u64(AtomicU64 *atomic, U64 old_value, U64 new_value){
  LONG64 init_value = InterlockedCompareExchange64((volatile LONG64*)&atomic->_value, new_value, old_value);
  return init_value == old_value;
}

#else
#error "Unknown compiler"
#endif

//
// STRINGS
//

// `String` = UTF-8 encoded immutable string; `Buffer` is just immutable raw non-textual data
typedef struct String String, Buffer;
struct String{
  U8 *data;
  I64 size; // NOTE: We do 64 bit signed lengths because that simplifies a lot of stuff and makes clamping values reasonable
};
CT_ASSERT(sizeof(String) == 16);

#if !COMPILER_GCC
#define LIT_STR(...) ( string_create((U8*)(__VA_ARGS__), (size_t)sizeof(__VA_ARGS__) - 1) )
#else
#define LIT_STR(...) ( (String){(U8*)(__VA_ARGS__), (size_t)sizeof(__VA_ARGS__) - 1} )
#endif

#define StrFormatArg(_String_) ((int)(_String_).size), ((char*)(_String_).data)

static String string_create(U8 *data, I64 size){
  String result = {0};
  assert(size >= 0);
  result.data = data;
  result.size = size;
  return result;
}
static String string_nil(){
  String result = {0};
  return result;
}

static String string_from_cstring(const char *cstring){
  String result = {0};
  result.data = (U8*)cstring;
  size_t size = 0;
  while(*cstring++){ size += 1; }
  result.size = size;
  return result;
}
static bool string_to_cstring(String string, char *buffer, size_t size){
  bool result = false;

  if(string.size + 1 <= size){
    result = true;
    memcpy(buffer, string.data, string.size);
    buffer[string.size] = '\0';
  }

  return result;
}

static String local_printf(U8 *buffer, size_t size, const char *format, ...){
  va_list list;
  va_start(list, format);
  assert(size < INT32_MAX);
  stbsp_vsnprintf((char*)buffer, (int)size, format, list);
  va_end(list);
  String result = string_from_cstring((char*)buffer);
  return result;
}
#define LOCAL_PRINTF(_name_, _size_, ...) U8 COMBINE2(_name_, __buffer)[(_size_)]; String _name_ = local_printf(COMBINE2(_name_, __buffer), sizeof(COMBINE2(_name_, __buffer)), __VA_ARGS__)

static String substring_nocheck(String string, I64 begin, I64 end){
  assert(begin >= 0);
  assert(end >= 0);
  String result = {0};
  if(string.size >= end && end > begin){
    result.data = string.data + begin;
    result.size = end - begin;
  }
  return result;
}
static String substring(String string, I64 begin, I64 end){
  assert(end >= begin);
  return substring_nocheck(string, begin, end);
}

static bool string_equals(String a, String b){
  bool result = false;
  if(a.data && b.data && a.size == b.size && a.size > 0){
    result = true;
    for(U64 index = 0; index < a.size; ++index){
      if(a.data[index] != b.data[index]){
        result = false;
        break;
      }
    }
  }
  return result;
}

static bool cstring_equals(char *a, char *b){
  bool result = true;
  while(true){
    if(*a != *b){ result = false; break; }
    else if(*a == '\0') break;
    ++a; ++b;
  }
  return result;
}

static bool string_starts_with(String string, String with){
  return string_equals(substring(string, 0, with.size), with);
}
static bool string_skip(String string, I64 *_cursor, String with){
  I64 cursor = *_cursor;

  bool result = false;
  if(cursor + with.size < string.size && string_equals(substring(string, cursor, cursor + with.size), with)){
    result = true;
    cursor += with.size;
  }

  *_cursor = cursor;
  return result;
}

static bool string_consume(String *string, String with){
  bool result = string_starts_with(*string, with);
  if(result){
    *string = substring(*string, with.size, string->size);
  }
  return result;
}

static String string_strip_spaces(String value){
  String result = value;

  while(result.size > 0 && result.data[0] == ' '){
    result = substring(result, 1, result.size);
  }

  while(result.size > 0 && result.data[result.size - 1] == ' '){
    result = substring(result, 0, result.size - 1);
  }

  return result;
}

static bool split_string_in_two_at_ascii_character(String string, char ascii, String *before, String *after){
  bool result = false;
  U32 index = 0;
  for(; index < string.size; ++index){
    if(string.data[index] == ascii){
      break;
    }
  }

  if(index < string.size){
    result = true;

    *before = substring(string, 0, index);
    *after = substring(string, index + 1, string.size);
  }

  return result;
}

//~ UTF8 encoding/decoding

#define INVALID_CODEPOINT 0xffffffff

static bool is_utf8_continuation_byte(U8 value){
  bool result = false;
  if((value & 0xc0) == 0x80){
    result = true;
  }
  return result;
}

static U32 decode_utf8(U8 *input_begin, U8 *input_end, U32 *_encoded_size){
  U32 result = INVALID_CODEPOINT;
  U32 encoded_size = 0;

  if(input_begin < input_end){
    U8 value = input_begin[0];

    U8 masks[4]    = { 0x80, 0xe0, 0xf0, 0xf8 };
    U8 patterns[4] = { 0x00, 0xc0, 0xe0, 0xf0 };

    for(int index = 0; index < 4; ++index){
      if((value & masks[index]) == patterns[index]){
        if(input_begin + index + 1 <= input_end){
          encoded_size = index + 1;
        }
        break;
      }
    }

    if(encoded_size){
      result = value & ~masks[encoded_size - 1];

      for(U32 index = 1; index < encoded_size; ++index){
        result <<= 6;
        result |= input_begin[index] & 0x3f;
      }
    }
  }

  if(_encoded_size) *_encoded_size = encoded_size;
  return result;
}

// NOTE: Returns the number of bytes that were written to the buffer.
static U32 encode_utf8(U32 codepoint_init, U8 *buffer_begin, U8 *buffer_end){
  // TODO(hanna - 2019-03-22): This is highly untested! Use at your own risk!
  U32 result = 0;
  if(codepoint_init <= 0x7f && buffer_begin + 1 <= buffer_end){
    result = 1;
    buffer_begin[0] = codepoint_init;
  }else if(codepoint_init <= 0x7ff && buffer_begin + 2 <= buffer_end){
    result = 2;
  }else if(codepoint_init <= 0xffff && buffer_begin + 3 <= buffer_end){
    result = 3;
  }else if(codepoint_init <= 0x10ffff && buffer_begin + 4 <= buffer_end){
    result = 4;
  }

  if(result > 1){
    U32 codepoint = codepoint_init;
    buffer_begin[0] = 0x80;
    for(int index = result - 1; index > 0; --index){
      buffer_begin[0] |= 1 << (7 - index);
      buffer_begin[index] = 0x80 | (codepoint & 0x3f);
      codepoint >>= 6;
    }
    buffer_begin[0] |= codepoint;
  }

  return result;
}

//~ String decoding utility

// Peek codepoint in UTF-8 encoded string
static U32 peek_codepoint(String string, I64 cursor){
  return decode_utf8(string.data + cursor, string.data + string.size, NULL);
}
static bool next_codepoint(String string, I64 *_cursor){
  I64 cursor = *_cursor;

  bool result = false;
  if(cursor < string.size){
    result = true;

    U32 encoded_length;
    decode_utf8(string.data + cursor, string.data + string.size, &encoded_length);
    if(encoded_length){
      cursor += encoded_length;
    }else{
      cursor += 1;
    }
  }
  *_cursor = cursor;
  return result;
}
static bool prev_codepoint(String string, I64 *_cursor){
  I64 cursor = *_cursor;
  bool result = false;
  if(cursor > 0){
    result = true;
    cursor -= 1;
    while(cursor > 0 && is_utf8_continuation_byte(string.data[cursor])){
      cursor -= 1;
    }
  }
  *_cursor = cursor;
  return result;
}

//

/*
for(UTF8Iterator iter = iterate_utf8(string);
    iter.valid;
    advance_utf8_iterator(&iter))
{
}
*/
typedef struct UTF8Iterator UTF8Iterator;
struct UTF8Iterator{
  // INTERNALS
  U8 *begin;
  I64 at;
  U8 *end;

  // PUBLIC
  bool valid;
  I64 byte_index;
  I64 codepoint_index;
  U32 codepoint;
  U32 codepoint_bytes;
};
static UTF8Iterator iterate_utf8(String string);
static void advance_utf8_iterator(UTF8Iterator *iter);

static UTF8Iterator iterate_utf8(String string){
  UTF8Iterator result = {0};
  result.begin = string.data;
  result.end = string.data + string.size;
  result.codepoint_index = -1;
  advance_utf8_iterator(&result);
  return result;
}
static void advance_utf8_iterator(UTF8Iterator *iter){
  iter->codepoint = decode_utf8(iter->begin + iter->at, iter->end, &iter->codepoint_bytes);
  iter->byte_index = iter->at;
  iter->at += iter->codepoint_bytes;
  iter->valid = (iter->codepoint_bytes != 0);
  iter->codepoint_index += 1;
}

static I64 utf8_get_codepoint_count(String string){
  UTF8Iterator iter;
  for(iter = iterate_utf8(string);
      iter.valid;
      advance_utf8_iterator(&iter));
  return iter.codepoint_index;
}

//~ String navigation for text editing UI

static I64 string_correct_cursor(String string, I64 pos){
  I64 result = CLAMP(0, pos, string.size);
  while(0 < result && result < string.size && is_utf8_continuation_byte(string.data[result])){
    result -= 1;
  }
  return result;
}

static I64 string_move_left(String string, I64 pos){
  I64 result = pos;

  if(0 < result && result <= string.size){
    result -= 1;
    while(0 < result && is_utf8_continuation_byte(string.data[result])){
      result -= 1;
    }
  }
  return result;
}
static I64 string_move_right(String string, I64 pos){
  I64 result = pos;

  if(0 <= result && result < string.size){
    result += 1;
    while(result < string.size && is_utf8_continuation_byte(string.data[result])){
      result += 1;
    }
  }

  return result;
}

static bool _string_move_is_whitespace(char c){
  return (c == ' ' || c == '\t');
}
static bool _string_move_is_word_char(char c){
  return ('a' <= c && c <= 'z')
      || ('A' <= c && c <= 'Z')
      || ('0' <= c && c <= '9')
      || (c == '_');
}
static I64 string_move_left_word(String string, I64 pos){
  I64 result = pos;

  if(0 < result && result <= string.size){
    while(0 < result && _string_move_is_whitespace(string.data[result - 1])){
      result -= 1;
    }

    while(0 < result && _string_move_is_word_char(string.data[result - 1])){
      result -= 1;
    }

    if(result == pos){
      result = pos - 1;
    }
  }

  return result;
}
static I64 string_move_right_word(String string, I64 pos){
  assert(0 <= pos);
  I64 result = pos;

  if(result < string.size){
    while(result < string.size && _string_move_is_whitespace(string.data[result])){
      result += 1;
    }

    while(result < string.size && _string_move_is_word_char(string.data[result])){
      result += 1;
    }

    if(result == pos){
      result = pos + 1;
    }
  }

  return result;
}

// *******************************************
//=============================================
// BEGIN OS LAYER CODE OS INTERACTION CODE OS API
//=============================================
// *******************************************

typedef struct Allocator Allocator;

typedef struct OSFile{ U64 value; }OSFile;
typedef struct OSMappedFile{ void *data; I64 data_size; }OSMappedFile;
typedef struct OSThread{ U64 value; }OSThread;

static void* os_alloc_pages_commit(size_t size); // Returned pointer is NULL on failure; otherwise a pointer to commited zeroed pages is returned.
static void* os_alloc_pages_nocommit(size_t size); // Returned page is NULL on failure; otherwise a pointer to uncommited zeroed pages is returned.
static void os_free_pages(void *memory, size_t size);
static OSFile os_open_file_input(String path);
static OSFile os_open_file_output(String path);
static void os_close_file(OSFile file);
static I64 os_get_file_size(OSFile file);
static U64 os_get_file_modify_time_us(OSFile file); // In unix time in microseconds.
static bool os_read_from_file(OSFile file, I64 offset, U8 *buffer, U32 size);
/*
NOTE(hanna - 2020-01-26): The intended usage pattern of this is something like the following:

bool error = false;
uint64_t at = 0;
os_write_to_file(file, at, buffer, size, &error);
at += size;
...
os_write_to_file(file, at, buffer, size, &error);
at += size;
if(error){
// A write has failed!
// If 'error == true' then further calls to os_write_to_file are ignored.
}

If 'error = null' an attempt will always be made to perform the write, i.e. it has the same bahaviour as `*error = true`.
*/
static void os_write_to_file(OSFile file, I64 offset, U8 *buffer, U32 size, bool *error);

static OSMappedFile os_begin_memory_map_file_readonly(OSFile file);
static void os_end_memory_map_file(OSFile file, OSMappedFile mapped_file);

typedef struct OSDir OSDir;
struct OSDir{
  bool success;
  String *entry_filenames;
  I64 entry_count;
};
static OSDir os_read_directory_entries(String path, Allocator *allocator);
static bool os_create_directory(String path);

typedef struct OSPathInfo OSPathInfo;
struct OSPathInfo{
#define OS_PATH_KIND_error           0 /* Underlying call into OS failed */
#define OS_PATH_KIND_does_not_exist  1
#define OS_PATH_KIND_file            2
#define OS_PATH_KIND_directory       3
#define OS_PATH_KIND_other           4
  EnumU32(OS_PATH_INFO_KIND) kind;
  U64 unix_modify_time_us;
};
static OSPathInfo os_get_path_info(String path);

static U64 os_get_monotonic_time_us(); // NOTE: Both monotonically increasing & continuous. Relative to an arbitrary point in time.
static U64 os_get_unix_time_us(); // NOTE: System time. Need not be continuous.
static I64 os_get_unix_time();

static String os_get_working_directory(Allocator *allocator);
static String os_get_home_directory(Allocator *allocator);

static OSThread os_start_thread(void (*entry_point)(void*), void *userdata, String name);
static void os_join_thread(OSThread thread);
static OSThread os_get_handle_to_current_thread();
static void os_sleep_us(uint64_t duration);

static U64 os_get_entropy_u64();

//=====================
// END PLATFORM CODE
//=====================

//
// ThreadContext
//

typedef struct ThreadContext ThreadContext;
struct ThreadContext{
  // Each entry is a _singly_ linked list of pages to be used by the allocator
  // size of page at index i is OS_PAGE_SIZE * 2^i
  struct AllocatorPageHeader *page_cache_first[4];

  // The currently active allocator budget, used by all temp allocators
  struct AllocatorBudget *budget;

  //~ Variables for the symbols project
  struct{
    struct Allocator *frame_temp;
    struct ProfThreadContext *prof;
  } symbols;

  struct{
    struct SpallBuffer *spall_buffer;
    U32 spall_tid;
  } spall;
};
static void set_thread_context(ThreadContext *tc);
static ThreadContext* get_thread_context();

#if COMPILER_GCC
static __thread ThreadContext *_global_thread_context;

static void set_thread_context(ThreadContext *tc){
  _global_thread_context = tc;
}
static ThreadContext* get_thread_context(){
  assert(_global_thread_context);
  return _global_thread_context;
}
#elif COMPILER_MSVC
static __declspec(thread) ThreadContext *_global_thread_context;

static void set_thread_context(ThreadContext *tc){
  _global_thread_context = tc;
}
static ThreadContext* get_thread_context(){
  assert(_global_thread_context);
  return _global_thread_context;
}
#else
#error "Unsupported compiler"
#endif


//
// MEMORY MANAGEMENT
//

/////////////////////////////////////////////////////////////////////////////
//~ Allocator interface (NOTE NOTE NOTE THAT THIS IS NOT MULTITHREAD SAFE!!!)

/*
TODO(hanna - 2022-12-08): Our big todos are

[ ] TODO: Thread local cache of pages (and think about DDoS opportunities through that system)
[x] TODO: Temporary memory storage

*/

//~ Allocator Budget

typedef struct AllocatorBudget AllocatorBudget;
struct AllocatorBudget{
  size_t max_num_used_bytes;
  size_t used_bytes_watermark; // The highest `used_bytes` ever has been
  size_t used_bytes;

  bool oom_handler_set;
  jmp_buf oom_handler;
};

static void allocator_budget_signal_out_of_memory(AllocatorBudget *budget){
  assert(budget->oom_handler_set);
  longjmp(budget->oom_handler, 1);
}
static void allocator_budget_use_bytes(AllocatorBudget *budget, size_t bytes){
  assert(budget->oom_handler_set);
  if(budget->used_bytes + bytes > budget->max_num_used_bytes){
    allocator_budget_signal_out_of_memory(budget);
  }
  budget->used_bytes += bytes;
  budget->used_bytes_watermark = MAXIMUM(budget->used_bytes_watermark, budget->used_bytes);
}
static void allocator_budget_release_bytes(AllocatorBudget *budget, size_t bytes){
  //assert(budget->oom_handler_set); You may release bytes even when not in a begin/end pair
  assert(budget->used_bytes >= bytes && "Releasing more bytes than there are!!");
  budget->used_bytes -= bytes;
}
static void _allocator_budget_begin(AllocatorBudget *budget, size_t max_num_used_bytes){
  ThreadContext *tc = get_thread_context();
  assert(!tc->budget);
  tc->budget = budget;

  assert(!budget->oom_handler_set);
  budget->oom_handler_set = true;
  budget->max_num_used_bytes = max_num_used_bytes;
}
#define allocator_budget_begin(_budget_, _max_num_used_bytes_) \
  ( _allocator_budget_begin((_budget_), (_max_num_used_bytes_)), setjmp((_budget_)->oom_handler) ) \

static void allocator_budget_end(AllocatorBudget *budget){
  ThreadContext *tc = get_thread_context();
  assert(tc->budget);
  tc->budget = NULL;

  assert(budget->oom_handler_set);
  budget->oom_handler_set = false;
}

//~ Heap alloc

typedef struct HeapAllocHeader HeapAllocHeader;
struct HeapAllocHeader{
  U16 size : 15; // Size, including any padding, i.e. this is always the offset to the next HeapAllocHeader in the list (excluding the last entry, that is)
  U16 occupied : 1;
  U8 data[0];
};
CT_ASSERT(sizeof(HeapAllocHeader) == 2);


//~ Allocator

#define ALLOCATOR_MAX_ALIGN 64

typedef struct AllocatorPageHeader AllocatorPageHeader;
struct AllocatorPageHeader{
  Allocator *allocator;
  AllocatorPageHeader *next;
  AllocatorPageHeader *prev;
  size_t page_count;
};
CT_ASSERT(sizeof(AllocatorPageHeader) == 32); // just stating the size explicitly here,

static U8* allocator_page_header_data(AllocatorPageHeader *header){
  return (U8*)header + sizeof(AllocatorPageHeader);
}

#define BIG_ALLOC_SIZE (OS_PAGE_SIZE - 64 + 1)
CT_ASSERT(BIG_ALLOC_SIZE <= OS_PAGE_SIZE - sizeof(AllocatorPageHeader) - sizeof(HeapAllocHeader) + 1); // Max alloc size for the heap allocator

static AllocatorPageHeader* get_allocator_page(void *alloc){
  assert(alloc);
  return (AllocatorPageHeader*)(((uintptr_t)alloc) & ~(uintptr_t)(OS_PAGE_SIZE - 1));
}
static Allocator* get_allocator(void *alloc){
  Allocator *result = get_allocator_page(alloc)->allocator;
  assert(result);
  return result;
}

//~ Temporary alloc

CT_ASSERT(OS_PAGE_SIZE == 4096);
#define TEMP_ALLOC_UNIT LIT_U64(64)
#define TEMP_ALLOC_UNIT_LOG2 LIT_U64(6)
CT_ASSERT(ALLOCATOR_MAX_ALIGN <= TEMP_ALLOC_UNIT); // All allocations on the temporary memory space are aligned to TEMP_ALLOC_UNIT

typedef struct TempPage TempPage;
struct TempPage{
  AllocatorPageHeader header;
  U64 free_list_memory_order; // NOTE: last bit should always be set, as that corresponds to invalid memory (because this header uses 64 bytes of memory we don't have room for 64 blocks of 64 bytes, only 63 blocks of 64 bytes).
  U8 _padding[24]; // Make sure we have the right alignment
  U8 data[OS_PAGE_SIZE - 64];
};
CT_ASSERT(sizeof(TempPage) == OS_PAGE_SIZE);

//~ Heap alloc

static uintptr_t align_address_with_header(uintptr_t ptr, uintptr_t align, uintptr_t header_size){
  uintptr_t result = ptr;

  assert(align <= ALLOCATOR_MAX_ALIGN);
  assert(IS_POWER_OF_TWO(align));

  // First attempt to align and see if we have space for the header
  result = (result + align - 1) & ~(align - 1);
  assert((result & (align - 1)) == 0);

  while(result - ptr < header_size){
    // We don't have space for the header, so we must go to the next address which is a mulitple of `align`.
    result += align;
  }
  assert(result - ptr >= header_size);
  assert((result & (align - 1)) == 0);

  return result;
}

static HeapAllocHeader* _heap_alloc_header_from_ptr(void *ptr){
  assert(ptr);
  HeapAllocHeader *result = (HeapAllocHeader*)((U8*)ptr - sizeof(HeapAllocHeader));
  return result;
}
static HeapAllocHeader* _heap_alloc_next(AllocatorPageHeader *page, HeapAllocHeader *alloc){
  HeapAllocHeader *result = (HeapAllocHeader*)((uintptr_t)alloc + sizeof(HeapAllocHeader) + alloc->size);
  if((uintptr_t)result >= (uintptr_t)page + OS_PAGE_SIZE){
    result = NULL;
  }
  return result;
}

//~ Big alloc header

typedef struct BigAllocHeader BigAllocHeader;
struct BigAllocHeader{
  AllocatorPageHeader page_header;
  size_t size; // size of data
  // ... padding for alignment reasons.
  // then data
};

//~ Push chunk header

typedef struct PushChunkHeader PushChunkHeader;
struct PushChunkHeader{
  AllocatorPageHeader page_header;
};
static U8* push_chunk_data(PushChunkHeader *header){
  return (U8*)header + sizeof(PushChunkHeader);
}

//~ And, finally, the definition of the Allocator struct!

typedef struct Allocator Allocator;
struct Allocator{
  #define ALLOCATOR_KIND_heap            1 /* long term storage */
  #define ALLOCATOR_KIND_temporary       2 /* temporary scratch space, optimize for growing allocations which are all freed at the same time */
  U32 kind;

  AllocatorBudget *budget;

  //~ Alloc/Free interface
  AllocatorPageHeader heap_sentinel;
  AllocatorPageHeader temp_sentinel;
  BigAllocHeader big_alloc_sentinel;

  //~ Push allocator
  uintptr_t push_cursor;
  PushChunkHeader push_sentinel;

  //~ Stub
  AllocatorPageHeader *stub;
};

//~ Allocator API

// TODO: Put the other declarations here
static void* allocator_get_stub(Allocator *allocator);

//~ Allocator common code

/*
TODO?: A NOTE(hanna) about the page cache: For now we don't support using the page cache when simultaneously using a allocator budget
This shouldn't be a problem for as the only project using the allocator budget system is the web server, which is not performance-oriented
at all.

Currently on a test workload (running the euclid_minmax routine for calculating Ulf Adam's k factor) which is
heavy with big int work we get the following result with the page cache turned off:
Took 72.133s
With the page cache turned on we get:
Took 35.902s
(With some further modifications to the program and with a bug in the page cache fixed: Took 21.255s)

A big TODO with this system: Start evicting pages from the cache.
*/
#define ALLOCATOR_ENABLE_PAGE_CACHE 1

static U64 _allocator_get_cache_index(U64 page_count){
  assert(page_count > 0);
  if(page_count == 1){
    return 0;
  }else{
    return index_of_high_bit_u64(page_count - 1) + 1;
  }
}

static AllocatorPageHeader* _allocator_alloc_pages(Allocator *allocator, AllocatorPageHeader *sentinel, U64 given_page_count){
  assert(given_page_count > 0);

  ThreadContext *tc = get_thread_context();

  AllocatorPageHeader *result = NULL;
#if ALLOCATOR_ENABLE_PAGE_CACHE
  if(!allocator->budget){
    U64 cache_index = _allocator_get_cache_index(given_page_count);
    if(cache_index < ARRAY_COUNT(tc->page_cache_first) && tc->page_cache_first[cache_index]){
      result = tc->page_cache_first[cache_index];
      tc->page_cache_first[cache_index] = result->next;
      assert(result->page_count >= given_page_count);
    }
  }
#endif

  if(!result){
    U64 page_count = given_page_count;
#if ALLOCATOR_ENABLE_PAGE_CACHE
    // NOTE(hanna): Here we round up the size of allocations to allow easy searching through the cache
    U64 cache_index = _allocator_get_cache_index(page_count);
    if(!allocator->budget && cache_index < ARRAY_COUNT(tc->page_cache_first)){
      page_count = LIT_U64(1) << cache_index;
    }
#endif
    U64 size = page_count * OS_PAGE_SIZE;

    if(allocator->budget){
      allocator_budget_use_bytes(allocator->budget, size);
    }
    result = (AllocatorPageHeader*)os_alloc_pages_commit(size);
    result->page_count = page_count;
    if(!result){
      if(allocator->budget){
        allocator_budget_signal_out_of_memory(allocator->budget);
        // TODO: Log a warning?
      }else{
        panic("Allocator ran out of memory and there is no out of memory handler installed! Tried to allocate %I64u bytes.", size);
      }
    }
  }

  assert(result->page_count >= given_page_count);

  result->allocator = allocator;

  if(sentinel){
    result->next = sentinel->next;
    result->next->prev = result;
    result->prev = sentinel;
    result->prev->next = result;
  }

  return result;
}
static void _allocator_free_pages_no_unlink(Allocator *allocator, AllocatorPageHeader *page){
  ThreadContext *tc = get_thread_context();

#if ALLOCATOR_ENABLE_PAGE_CACHE
  if(!allocator->budget){
    U64 cache_index = _allocator_get_cache_index(page->page_count);

    if(cache_index < ARRAY_COUNT(tc->page_cache_first)){
      page->next = tc->page_cache_first[cache_index];
      tc->page_cache_first[cache_index] = page;
    }else{
      os_free_pages(page, page->page_count * OS_PAGE_SIZE);
    }
  }else
#endif
  {
    os_free_pages(page, page->page_count * OS_PAGE_SIZE);
    if(allocator->budget){
      allocator_budget_release_bytes(allocator->budget, page->page_count * OS_PAGE_SIZE);
    }
  }
}
static void _allocator_free_pages_unlink(Allocator *allocator, AllocatorPageHeader *page){
  page->prev->next = page->next;
  page->next->prev = page->prev;
  _allocator_free_pages_no_unlink(allocator, page);
}

static Allocator* _allocator_make(AllocatorBudget *budget, U32 kind){
  Allocator *result = (Allocator*)calloc(1, sizeof(Allocator)); // TODO: Don't use calloc here, allocate us on the stub
  if(!result){
    panic("Ran out of memory :-(");
  }
  result->kind = kind;
  result->budget = budget;
  result->big_alloc_sentinel.page_header.next = result->big_alloc_sentinel.page_header.prev = &result->big_alloc_sentinel.page_header;
  result->push_sentinel.page_header.next = result->push_sentinel.page_header.prev = &result->push_sentinel.page_header;
  result->stub = _allocator_alloc_pages(result, NULL, 1); // TODO: Check that none of the memory is tampered with?
  return result;
}

//~ Big alloc

static void* _big_alloc_alloc(Allocator *allocator, size_t size, size_t align){
  assert(align < OS_PAGE_SIZE && "You are aligning a little bit much");
  size_t header_size = ((sizeof(BigAllocHeader) + align - 1) & ~(align - 1));

  size_t total_size = header_size + size;
  total_size = (total_size + OS_PAGE_SIZE - 1) & ~(size_t)(OS_PAGE_SIZE - 1);
  U64 page_count = total_size >> OS_PAGE_SIZE_LOG2;

  BigAllocHeader *new_alloc = (BigAllocHeader*)_allocator_alloc_pages(allocator, &allocator->big_alloc_sentinel.page_header, page_count);

  new_alloc->size = size;

  void *result = (void*)((U8*)new_alloc + header_size);
  assert(((uintptr_t)result & (uintptr_t)(align - 1)) == 0 && "Big alloc not properly aligned!");
  return result;
}

static void _allocator_free_all_pages(Allocator *allocator, AllocatorPageHeader *sentinel){
  for(AllocatorPageHeader *page = sentinel->next; page != sentinel;){
      AllocatorPageHeader *next = page->next;
      _allocator_free_pages_no_unlink(allocator, page);
      page = next;
    }
}

static void allocator_destroy(Allocator **_allocator){
  // NOTE: It is super-important that this is done first, because _allocator may be
  // part of what is being freed!
  Allocator *allocator = *_allocator;
  *_allocator = NULL;

  if(allocator){
    if(allocator->kind == ALLOCATOR_KIND_heap){
      _allocator_free_all_pages(allocator, &allocator->heap_sentinel);
    }else if(allocator->kind == ALLOCATOR_KIND_temporary){
      _allocator_free_all_pages(allocator, &allocator->temp_sentinel);
    }else{
      panic("Unknown allocator kind!");
    }
    _allocator_free_all_pages(allocator, &allocator->big_alloc_sentinel.page_header);
    _allocator_free_all_pages(allocator, &allocator->push_sentinel.page_header);

    _allocator_free_pages_no_unlink(allocator, allocator->stub);
    free(allocator);
  }
}

//~ Heap allocator
// This is a super naive heap allocator that is not designed to be efficient or good in any way.
// It was created because malloc didn't meet the requirements posed by the web server project.
// - hanna 2022-09-08

//
// TODO(hanna - 2022-12-04): Lets replace the old allocator with something that actually scales to more allocations
//
// Lets review the literature on the subject. Here as some references:
// jemalloc --> A Scalable Concurrent malloc(3) Implementation for FreeBSD --> https://people.freebsd.org/~jasone/jemalloc/bsdcan2006/jemalloc.pdf
// gingerbill --> https://www.gingerbill.org/series/memory-allocation-strategies/
//

static void* _heap_alloc_on_page(AllocatorPageHeader *page, size_t size, size_t align){
  Allocator *allocator = page->allocator;
  assert(allocator->kind == ALLOCATOR_KIND_heap);

  void *result = NULL;

  // NOTE(hanna): The first allocation is special and is used for proper padding when the first allocation has some padding requirement on it.
  HeapAllocHeader *first_alloc = (HeapAllocHeader*)allocator_page_header_data(page);
  HeapAllocHeader *prev = first_alloc;
  HeapAllocHeader *alloc = _heap_alloc_next(page, first_alloc);
  while(alloc){
    // NOTE(hanna): What follows is just a stupid first fit and O(n), n=num allocations, allocator
    if(!alloc->occupied){
      while(true){ // Coalesce adjacent non-occupied regions.
        HeapAllocHeader *next = _heap_alloc_next(page, alloc);
        if(next && !next->occupied){
          alloc->size += sizeof(HeapAllocHeader) + next->size;
          assert((uintptr_t)alloc->data + alloc->size <= (uintptr_t)page + OS_PAGE_SIZE);
        }else{
          break;
        }
      }

      uintptr_t alloc_addr_begin = align_address_with_header((uintptr_t)alloc, (uintptr_t)align, (uintptr_t)sizeof(HeapAllocHeader));
      uintptr_t alloc_addr_end = alloc_addr_begin + size;
      uintptr_t region_addr_end = (uintptr_t)alloc + sizeof(HeapAllocHeader) + alloc->size;

      if(alloc_addr_end + sizeof(HeapAllocHeader) <= region_addr_end){
        // We found some memory to use for the allocation!
        prev->size = alloc_addr_begin - sizeof(HeapAllocHeader) - (uintptr_t)prev->data;

        alloc = (HeapAllocHeader*)(alloc_addr_begin - sizeof(HeapAllocHeader));
        clear_item(alloc);
        alloc->size = alloc_addr_end - alloc_addr_begin;
        alloc->occupied = 1;

        HeapAllocHeader *split_alloc = (HeapAllocHeader*)alloc_addr_end;
        clear_item(split_alloc);
        split_alloc->size = (U16)(region_addr_end - (alloc_addr_end + sizeof(HeapAllocHeader))),

        assert(region_addr_end >= alloc_addr_end + sizeof(HeapAllocHeader));
        assert(region_addr_end - (alloc_addr_end + sizeof(HeapAllocHeader)) <= UINT16_MAX);

        result = alloc->data;
        break;
      }
    }

    prev = alloc;
    alloc = _heap_alloc_next(page, alloc);
  }

  assert(((uintptr_t)result & (uintptr_t)(align -1)) == 0);

  return result;
}

static void* _heap_alloc(Allocator *allocator, size_t size, size_t align){
  assert(allocator->kind == ALLOCATOR_KIND_heap);

  void *result = NULL;
  for(AllocatorPageHeader *page = allocator->heap_sentinel.next; page != &allocator->heap_sentinel; page = page->next){
    result = _heap_alloc_on_page(page, size, align);
    if(result){
      break;
    }
  }

  if(!result){
    // NOTE(hanna): The first alloc is always occupied because it is used for alignment reasons.
    AllocatorPageHeader *page = _allocator_alloc_pages(allocator, &allocator->heap_sentinel, 1);
    HeapAllocHeader *first_alloc = (HeapAllocHeader*)allocator_page_header_data(page);
    first_alloc->occupied = true;
    first_alloc->size = 0;
    HeapAllocHeader *second_alloc = first_alloc + 1;
    second_alloc->occupied = false;
    second_alloc->size = ((uintptr_t)page + OS_PAGE_SIZE) - (uintptr_t)second_alloc->data;

    result = _heap_alloc_on_page(page, size, align);
    assert(result);
  }

  return result;
}

static void _heap_free(Allocator *allocator, void *ptr, size_t size){
  assert(allocator->kind == ALLOCATOR_KIND_heap);
  assert(ptr);

  AllocatorPageHeader *page = get_allocator_page(ptr);
  HeapAllocHeader *alloc = _heap_alloc_header_from_ptr(ptr);

  assert((void*)alloc != (void*)allocator_page_header_data(page) && "Cannot free the first allocation");
  alloc->occupied = false; // Coalescing happens in the allocation routine!
}

// NOTE: Destroyed with `allocator_destroy`
static Allocator* heap_allocator_make(AllocatorBudget *budget){
  Allocator *result = _allocator_make(budget, ALLOCATOR_KIND_heap);
  result->heap_sentinel.next = result->heap_sentinel.prev = &result->heap_sentinel;
  return result;
}

//
//~ Temporary allocator
//
// NOTE(hanna): This is an experiment with a new kind of bitmap allocator. It allocates things "maximally far away" from eachother
// so they have room to grow. What I mean by maximally far away from eachother is that the first allocation ends up at the beginning
// of the memory region, the second allocation exactly in the middle of the memory region, the third at 1/4, the fourth at 3/4, the
// next allocation at 1/8, then 3/8, 5/8, 7/8, 1/16, 3/16, etc.
//

static Allocator* temp_begin(void){
  ThreadContext *tc = get_thread_context();
  AllocatorBudget *budget = tc->budget;

  Allocator *result = _allocator_make(budget, ALLOCATOR_KIND_temporary);
  result->temp_sentinel.next = result->temp_sentinel.prev = &result->temp_sentinel;
  return result;
}
static void temp_end(Allocator **_allocator){
  assert(*_allocator && "This triggering indicates wrong usage");
  allocator_destroy(_allocator);
}

#if 0 // Freelist/memory conversion masks are generated by the following C program
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

typedef uint64_t U64;
#define fiz(_count_) for(U64 i = 0; i < (_count_); i += 1)
#define fjz(_count_) for(U64 j = 0; j < (_count_); j += 1)

static void print_bits(char *label, U64 v){
  printf("%10s: ", label);
  for(int i = 64; i --> 0;){
    if((v >> i) & 1){
      printf("1");
    }else{
      printf("0");
    }
  }
  printf(" (=%lu)", v);
  printf(" (=0x%lx)", v);
  printf("\n");
}
static U64 get_high_bit(U64 value){
  if(value){
    return 63 - __builtin_clzll(value);
  }else{
    return 0;
  }
}

U64 perm[64];

static U64 freelist_to_memory(U64 value){
  U64 result = 0;
  fiz(64){
    U64 bit = (value >> i) & 1;
    result |= bit << perm[i];
  }
  return result;
}
static U64 memory_to_freelist(U64 value){
  U64 result = 0;

  fjz(64){
    U64 bit = (value >> j) & 1;
    fiz(64){
      if(j == perm[i]){
        result |= bit << i;
        break;
      }
    }
  }

  return result;
}

#define TEST_IT 1
#if TEST_IT

U64 freelist_to_memory_table[16][16] = {
/* group  0 */ {0x0000000000000000, 0x0000000000000001, 0x0000000100000000, 0x0000000100000001, 0x0000000000010000, 0x0000000000010001, 0x0000000100010000, 0x0000000100010001, 0x0001000000000000, 0x0001000000000001, 0x0001000100000000, 0x0001000100000001, 0x0001000000010000, 0x0001000000010001, 0x0001000100010000, 0x0001000100010001, },
/* group  1 */ {0x0000000000000000, 0x0000000000000100, 0x0000000001000000, 0x0000000001000100, 0x0000010000000000, 0x0000010000000100, 0x0000010001000000, 0x0000010001000100, 0x0100000000000000, 0x0100000000000100, 0x0100000001000000, 0x0100000001000100, 0x0100010000000000, 0x0100010000000100, 0x0100010001000000, 0x0100010001000100, },
/* group  2 */ {0x0000000000000000, 0x0000000000000010, 0x0000000000001000, 0x0000000000001010, 0x0000000000100000, 0x0000000000100010, 0x0000000000101000, 0x0000000000101010, 0x0000000010000000, 0x0000000010000010, 0x0000000010001000, 0x0000000010001010, 0x0000000010100000, 0x0000000010100010, 0x0000000010101000, 0x0000000010101010, },
/* group  3 */ {0x0000000000000000, 0x0000001000000000, 0x0000100000000000, 0x0000101000000000, 0x0010000000000000, 0x0010001000000000, 0x0010100000000000, 0x0010101000000000, 0x1000000000000000, 0x1000001000000000, 0x1000100000000000, 0x1000101000000000, 0x1010000000000000, 0x1010001000000000, 0x1010100000000000, 0x1010101000000000, },
/* group  4 */ {0x0000000000000000, 0x0000000000000004, 0x0000000000000040, 0x0000000000000044, 0x0000000000000400, 0x0000000000000404, 0x0000000000000440, 0x0000000000000444, 0x0000000000004000, 0x0000000000004004, 0x0000000000004040, 0x0000000000004044, 0x0000000000004400, 0x0000000000004404, 0x0000000000004440, 0x0000000000004444, },
/* group  5 */ {0x0000000000000000, 0x0000000000040000, 0x0000000000400000, 0x0000000000440000, 0x0000000004000000, 0x0000000004040000, 0x0000000004400000, 0x0000000004440000, 0x0000000040000000, 0x0000000040040000, 0x0000000040400000, 0x0000000040440000, 0x0000000044000000, 0x0000000044040000, 0x0000000044400000, 0x0000000044440000, },
/* group  6 */ {0x0000000000000000, 0x0000000400000000, 0x0000004000000000, 0x0000004400000000, 0x0000040000000000, 0x0000040400000000, 0x0000044000000000, 0x0000044400000000, 0x0000400000000000, 0x0000400400000000, 0x0000404000000000, 0x0000404400000000, 0x0000440000000000, 0x0000440400000000, 0x0000444000000000, 0x0000444400000000, },
/* group  7 */ {0x0000000000000000, 0x0004000000000000, 0x0040000000000000, 0x0044000000000000, 0x0400000000000000, 0x0404000000000000, 0x0440000000000000, 0x0444000000000000, 0x4000000000000000, 0x4004000000000000, 0x4040000000000000, 0x4044000000000000, 0x4400000000000000, 0x4404000000000000, 0x4440000000000000, 0x4444000000000000, },
/* group  8 */ {0x0000000000000000, 0x0000000000000002, 0x0000000000000008, 0x000000000000000a, 0x0000000000000020, 0x0000000000000022, 0x0000000000000028, 0x000000000000002a, 0x0000000000000080, 0x0000000000000082, 0x0000000000000088, 0x000000000000008a, 0x00000000000000a0, 0x00000000000000a2, 0x00000000000000a8, 0x00000000000000aa, },
/* group  9 */ {0x0000000000000000, 0x0000000000000200, 0x0000000000000800, 0x0000000000000a00, 0x0000000000002000, 0x0000000000002200, 0x0000000000002800, 0x0000000000002a00, 0x0000000000008000, 0x0000000000008200, 0x0000000000008800, 0x0000000000008a00, 0x000000000000a000, 0x000000000000a200, 0x000000000000a800, 0x000000000000aa00, },
/* group 10 */ {0x0000000000000000, 0x0000000000020000, 0x0000000000080000, 0x00000000000a0000, 0x0000000000200000, 0x0000000000220000, 0x0000000000280000, 0x00000000002a0000, 0x0000000000800000, 0x0000000000820000, 0x0000000000880000, 0x00000000008a0000, 0x0000000000a00000, 0x0000000000a20000, 0x0000000000a80000, 0x0000000000aa0000, },
/* group 11 */ {0x0000000000000000, 0x0000000002000000, 0x0000000008000000, 0x000000000a000000, 0x0000000020000000, 0x0000000022000000, 0x0000000028000000, 0x000000002a000000, 0x0000000080000000, 0x0000000082000000, 0x0000000088000000, 0x000000008a000000, 0x00000000a0000000, 0x00000000a2000000, 0x00000000a8000000, 0x00000000aa000000, },
/* group 12 */ {0x0000000000000000, 0x0000000200000000, 0x0000000800000000, 0x0000000a00000000, 0x0000002000000000, 0x0000002200000000, 0x0000002800000000, 0x0000002a00000000, 0x0000008000000000, 0x0000008200000000, 0x0000008800000000, 0x0000008a00000000, 0x000000a000000000, 0x000000a200000000, 0x000000a800000000, 0x000000aa00000000, },
/* group 13 */ {0x0000000000000000, 0x0000020000000000, 0x0000080000000000, 0x00000a0000000000, 0x0000200000000000, 0x0000220000000000, 0x0000280000000000, 0x00002a0000000000, 0x0000800000000000, 0x0000820000000000, 0x0000880000000000, 0x00008a0000000000, 0x0000a00000000000, 0x0000a20000000000, 0x0000a80000000000, 0x0000aa0000000000, },
/* group 14 */ {0x0000000000000000, 0x0002000000000000, 0x0008000000000000, 0x000a000000000000, 0x0020000000000000, 0x0022000000000000, 0x0028000000000000, 0x002a000000000000, 0x0080000000000000, 0x0082000000000000, 0x0088000000000000, 0x008a000000000000, 0x00a0000000000000, 0x00a2000000000000, 0x00a8000000000000, 0x00aa000000000000, },
/* group 15 */ {0x0000000000000000, 0x0200000000000000, 0x0800000000000000, 0x0a00000000000000, 0x2000000000000000, 0x2200000000000000, 0x2800000000000000, 0x2a00000000000000, 0x8000000000000000, 0x8200000000000000, 0x8800000000000000, 0x8a00000000000000, 0xa000000000000000, 0xa200000000000000, 0xa800000000000000, 0xaa00000000000000, },
};

U64 memory_to_freelist_table[16][16] = {
/* group  0 */ {0x0000000000000000, 0x0000000000000001, 0x0000000100000000, 0x0000000100000001, 0x0000000000010000, 0x0000000000010001, 0x0000000100010000, 0x0000000100010001, 0x0000000200000000, 0x0000000200000001, 0x0000000300000000, 0x0000000300000001, 0x0000000200010000, 0x0000000200010001, 0x0000000300010000, 0x0000000300010001, },
/* group  1 */ {0x0000000000000000, 0x0000000000000100, 0x0000000400000000, 0x0000000400000100, 0x0000000000020000, 0x0000000000020100, 0x0000000400020000, 0x0000000400020100, 0x0000000800000000, 0x0000000800000100, 0x0000000c00000000, 0x0000000c00000100, 0x0000000800020000, 0x0000000800020100, 0x0000000c00020000, 0x0000000c00020100, },
/* group  2 */ {0x0000000000000000, 0x0000000000000010, 0x0000001000000000, 0x0000001000000010, 0x0000000000040000, 0x0000000000040010, 0x0000001000040000, 0x0000001000040010, 0x0000002000000000, 0x0000002000000010, 0x0000003000000000, 0x0000003000000010, 0x0000002000040000, 0x0000002000040010, 0x0000003000040000, 0x0000003000040010, },
/* group  3 */ {0x0000000000000000, 0x0000000000000200, 0x0000004000000000, 0x0000004000000200, 0x0000000000080000, 0x0000000000080200, 0x0000004000080000, 0x0000004000080200, 0x0000008000000000, 0x0000008000000200, 0x000000c000000000, 0x000000c000000200, 0x0000008000080000, 0x0000008000080200, 0x000000c000080000, 0x000000c000080200, },
/* group  4 */ {0x0000000000000000, 0x0000000000000004, 0x0000010000000000, 0x0000010000000004, 0x0000000000100000, 0x0000000000100004, 0x0000010000100000, 0x0000010000100004, 0x0000020000000000, 0x0000020000000004, 0x0000030000000000, 0x0000030000000004, 0x0000020000100000, 0x0000020000100004, 0x0000030000100000, 0x0000030000100004, },
/* group  5 */ {0x0000000000000000, 0x0000000000000400, 0x0000040000000000, 0x0000040000000400, 0x0000000000200000, 0x0000000000200400, 0x0000040000200000, 0x0000040000200400, 0x0000080000000000, 0x0000080000000400, 0x00000c0000000000, 0x00000c0000000400, 0x0000080000200000, 0x0000080000200400, 0x00000c0000200000, 0x00000c0000200400, },
/* group  6 */ {0x0000000000000000, 0x0000000000000020, 0x0000100000000000, 0x0000100000000020, 0x0000000000400000, 0x0000000000400020, 0x0000100000400000, 0x0000100000400020, 0x0000200000000000, 0x0000200000000020, 0x0000300000000000, 0x0000300000000020, 0x0000200000400000, 0x0000200000400020, 0x0000300000400000, 0x0000300000400020, },
/* group  7 */ {0x0000000000000000, 0x0000000000000800, 0x0000400000000000, 0x0000400000000800, 0x0000000000800000, 0x0000000000800800, 0x0000400000800000, 0x0000400000800800, 0x0000800000000000, 0x0000800000000800, 0x0000c00000000000, 0x0000c00000000800, 0x0000800000800000, 0x0000800000800800, 0x0000c00000800000, 0x0000c00000800800, },
/* group  8 */ {0x0000000000000000, 0x0000000000000002, 0x0001000000000000, 0x0001000000000002, 0x0000000001000000, 0x0000000001000002, 0x0001000001000000, 0x0001000001000002, 0x0002000000000000, 0x0002000000000002, 0x0003000000000000, 0x0003000000000002, 0x0002000001000000, 0x0002000001000002, 0x0003000001000000, 0x0003000001000002, },
/* group  9 */ {0x0000000000000000, 0x0000000000001000, 0x0004000000000000, 0x0004000000001000, 0x0000000002000000, 0x0000000002001000, 0x0004000002000000, 0x0004000002001000, 0x0008000000000000, 0x0008000000001000, 0x000c000000000000, 0x000c000000001000, 0x0008000002000000, 0x0008000002001000, 0x000c000002000000, 0x000c000002001000, },
/* group 10 */ {0x0000000000000000, 0x0000000000000040, 0x0010000000000000, 0x0010000000000040, 0x0000000004000000, 0x0000000004000040, 0x0010000004000000, 0x0010000004000040, 0x0020000000000000, 0x0020000000000040, 0x0030000000000000, 0x0030000000000040, 0x0020000004000000, 0x0020000004000040, 0x0030000004000000, 0x0030000004000040, },
/* group 11 */ {0x0000000000000000, 0x0000000000002000, 0x0040000000000000, 0x0040000000002000, 0x0000000008000000, 0x0000000008002000, 0x0040000008000000, 0x0040000008002000, 0x0080000000000000, 0x0080000000002000, 0x00c0000000000000, 0x00c0000000002000, 0x0080000008000000, 0x0080000008002000, 0x00c0000008000000, 0x00c0000008002000, },
/* group 12 */ {0x0000000000000000, 0x0000000000000008, 0x0100000000000000, 0x0100000000000008, 0x0000000010000000, 0x0000000010000008, 0x0100000010000000, 0x0100000010000008, 0x0200000000000000, 0x0200000000000008, 0x0300000000000000, 0x0300000000000008, 0x0200000010000000, 0x0200000010000008, 0x0300000010000000, 0x0300000010000008, },
/* group 13 */ {0x0000000000000000, 0x0000000000004000, 0x0400000000000000, 0x0400000000004000, 0x0000000020000000, 0x0000000020004000, 0x0400000020000000, 0x0400000020004000, 0x0800000000000000, 0x0800000000004000, 0x0c00000000000000, 0x0c00000000004000, 0x0800000020000000, 0x0800000020004000, 0x0c00000020000000, 0x0c00000020004000, },
/* group 14 */ {0x0000000000000000, 0x0000000000000080, 0x1000000000000000, 0x1000000000000080, 0x0000000040000000, 0x0000000040000080, 0x1000000040000000, 0x1000000040000080, 0x2000000000000000, 0x2000000000000080, 0x3000000000000000, 0x3000000000000080, 0x2000000040000000, 0x2000000040000080, 0x3000000040000000, 0x3000000040000080, },
/* group 15 */ {0x0000000000000000, 0x0000000000008000, 0x4000000000000000, 0x4000000000008000, 0x0000000080000000, 0x0000000080008000, 0x4000000080000000, 0x4000000080008000, 0x8000000000000000, 0x8000000000008000, 0xc000000000000000, 0xc000000000008000, 0x8000000080000000, 0x8000000080008000, 0xc000000080000000, 0xc000000080008000, },
};

static U64 nibble_based_freelist_to_memory(U64 value){
  U64 result = 0;
  fiz(16){
    U64 nibble = ((value >> (i * 4)) & 15);
    result |= freelist_to_memory_table[i][nibble];
  }
  return result;
}

static U64 nibble_based_memory_to_freelist(U64 value){
  U64 result = 0;
  fiz(16){
    U64 nibble = ((value >> (i * 4)) & 15);
    result |= memory_to_freelist_table[i][nibble];
  }
  return result;
}

#endif

int main(void){
  int N = 64;
  int Nlog2 = 6;
  U64 masks[6];
  for(U64 i = 0; i < N; i += 1){
    U64 high_bit = get_high_bit(i);
    U64 low = i - (1 << high_bit);
    U64 value = (N >> (high_bit + 1)) + (low << (Nlog2 - high_bit));
    if(i == 0) value = 0;
    print_bits("i", i);
    print_bits("value", value);
    perm[i] = value;
    printf("\n");
    masks[high_bit] |= ((U64)(1) << value);
  }
  for(U64 i = 0; i < Nlog2; i += 1){
    char buf[256]; sprintf(buf, "mask %lu", i);
    print_bits(buf, masks[i]);
  }
  printf("code:\n");
  for(U64 i = 0; i < Nlog2; i += 1){
    printf("LIT_U64(0x%lx),\n", masks[i]);
  }
  printf("perm:\n");
  for(U64 i = 0; i < N; i += 1){
    printf("%lu ", perm[i]);
    if((i + 1) % 8 == 0){ printf("\n"); }
  }

  // FREELIST TO MEMORY NIBBLES

  U64 group_bits = 4;
  printf("freelist_to_memory:\n");
  for(U64 group_index = 0; group_index < 64 / group_bits; group_index += 1){
    printf("/* group %2lu */ ", group_index);
    printf("{");
    fiz(16){
      printf("0x%016lx, ", freelist_to_memory(i << (group_index * group_bits)));
    }
    printf("},\n");
  }

  // MEMORY TO FREELIST

  printf("memory_to_freelist:\n");
  for(U64 group_index = 0; group_index < 64 / group_bits; group_index += 1){
    printf("/* group %2lu */ ", group_index);
    printf("{");
    fiz(16){
      printf("0x%016lx, ", memory_to_freelist(i << (group_index * group_bits)));
    }
    printf("},\n");
  }

#if TEST_IT
  // TEST IT

  fiz(1000000){
    U64 truth = freelist_to_memory(i);
    U64 value = nibble_based_freelist_to_memory(i);
    //printf("%lu --> %lx --> %lx\n", i, truth, value);

    assert(truth == value);
  }

  fiz(1000000){
    U64 truth = memory_to_freelist(i);
    U64 value = nibble_based_memory_to_freelist(i);
    //printf("%lu --> %lx --> %lx\n", i, truth, value);

    assert(truth == value);
  }

#endif // TEST_IT

  return 0;
}

#else
#if 0
/*
NOTE(hanna): In this version of the code we use BMI2 instructions PDEP and PEXT to permute our value.
*/

static U64 freelist_memory_masks[7] = {
  LIT_U64(0x0000000100000001),
  LIT_U64(0x0001000000010000),
  LIT_U64(0x0100010001000100),
  LIT_U64(0x1010101010101010),
  LIT_U64(0x4444444444444444),
  LIT_U64(0xaaaaaaaaaaaaaaaa),
};

static U64 _temp_freelist_to_memory(U64 value){
  // TODO: Optimization: According to Wikipedia and the internet, on AMD processors before Zen 3 PDEP/PEXT takes at least 18 cycles...
  // TODO: This also doesn't work on older processors, so we might want to find an alternative.
  //       This website seems like a good resource for doing bit permutations: https://programming.sirrida.de/bit_perm.html
  //       In the C program for generating `freelist_memory_masks` I also (untested) calculate the permutation for every subnibble
  //       which should allow us to calculate the permutation in ~16 cycles and only using 2kb of memory for the tables.
  U64 result = 0;
  result |= _pdep_u64(value >> LIT_U64(0), freelist_memory_masks[0]);
  result |= _pdep_u64(value >> LIT_U64(2), freelist_memory_masks[1]);
  result |= _pdep_u64(value >> LIT_U64(4), freelist_memory_masks[2]);
  result |= _pdep_u64(value >> LIT_U64(8), freelist_memory_masks[3]);
  result |= _pdep_u64(value >> LIT_U64(16), freelist_memory_masks[4]);
  result |= _pdep_u64(value >> LIT_U64(32), freelist_memory_masks[5]);
  return result;
}
static U64 _temp_memory_to_freelist(U64 value){
  // TODO: Optimization: According to Wikipedia and the internet, on AMD processors before Zen 3 PDEP/PEXT takes at least 18 cycles, or even worse, possibly over 200 cycles.
  U64 result = 0;
  result |= _pext_u64(value, freelist_memory_masks[0]) << LIT_U64(0);
  result |= _pext_u64(value, freelist_memory_masks[1]) << LIT_U64(2);
  result |= _pext_u64(value, freelist_memory_masks[2]) << LIT_U64(4);
  result |= _pext_u64(value, freelist_memory_masks[3]) << LIT_U64(8);
  result |= _pext_u64(value, freelist_memory_masks[4]) << LIT_U64(16);
  result |= _pext_u64(value, freelist_memory_masks[5]) << LIT_U64(32);
  return result;
}

#else
/*
NOTE(hanna): In this version of the code we map each perform the permutation for each nibble of the number
and use a lookup table for looking up what each transformed nibble becomes.
*/

static U64 freelist_to_memory_table[16][16] = {
/* group  0 */ {0x0000000000000000, 0x0000000000000001, 0x0000000100000000, 0x0000000100000001, 0x0000000000010000, 0x0000000000010001, 0x0000000100010000, 0x0000000100010001, 0x0001000000000000, 0x0001000000000001, 0x0001000100000000, 0x0001000100000001, 0x0001000000010000, 0x0001000000010001, 0x0001000100010000, 0x0001000100010001, },
/* group  1 */ {0x0000000000000000, 0x0000000000000100, 0x0000000001000000, 0x0000000001000100, 0x0000010000000000, 0x0000010000000100, 0x0000010001000000, 0x0000010001000100, 0x0100000000000000, 0x0100000000000100, 0x0100000001000000, 0x0100000001000100, 0x0100010000000000, 0x0100010000000100, 0x0100010001000000, 0x0100010001000100, },
/* group  2 */ {0x0000000000000000, 0x0000000000000010, 0x0000000000001000, 0x0000000000001010, 0x0000000000100000, 0x0000000000100010, 0x0000000000101000, 0x0000000000101010, 0x0000000010000000, 0x0000000010000010, 0x0000000010001000, 0x0000000010001010, 0x0000000010100000, 0x0000000010100010, 0x0000000010101000, 0x0000000010101010, },
/* group  3 */ {0x0000000000000000, 0x0000001000000000, 0x0000100000000000, 0x0000101000000000, 0x0010000000000000, 0x0010001000000000, 0x0010100000000000, 0x0010101000000000, 0x1000000000000000, 0x1000001000000000, 0x1000100000000000, 0x1000101000000000, 0x1010000000000000, 0x1010001000000000, 0x1010100000000000, 0x1010101000000000, },
/* group  4 */ {0x0000000000000000, 0x0000000000000004, 0x0000000000000040, 0x0000000000000044, 0x0000000000000400, 0x0000000000000404, 0x0000000000000440, 0x0000000000000444, 0x0000000000004000, 0x0000000000004004, 0x0000000000004040, 0x0000000000004044, 0x0000000000004400, 0x0000000000004404, 0x0000000000004440, 0x0000000000004444, },
/* group  5 */ {0x0000000000000000, 0x0000000000040000, 0x0000000000400000, 0x0000000000440000, 0x0000000004000000, 0x0000000004040000, 0x0000000004400000, 0x0000000004440000, 0x0000000040000000, 0x0000000040040000, 0x0000000040400000, 0x0000000040440000, 0x0000000044000000, 0x0000000044040000, 0x0000000044400000, 0x0000000044440000, },
/* group  6 */ {0x0000000000000000, 0x0000000400000000, 0x0000004000000000, 0x0000004400000000, 0x0000040000000000, 0x0000040400000000, 0x0000044000000000, 0x0000044400000000, 0x0000400000000000, 0x0000400400000000, 0x0000404000000000, 0x0000404400000000, 0x0000440000000000, 0x0000440400000000, 0x0000444000000000, 0x0000444400000000, },
/* group  7 */ {0x0000000000000000, 0x0004000000000000, 0x0040000000000000, 0x0044000000000000, 0x0400000000000000, 0x0404000000000000, 0x0440000000000000, 0x0444000000000000, 0x4000000000000000, 0x4004000000000000, 0x4040000000000000, 0x4044000000000000, 0x4400000000000000, 0x4404000000000000, 0x4440000000000000, 0x4444000000000000, },
/* group  8 */ {0x0000000000000000, 0x0000000000000002, 0x0000000000000008, 0x000000000000000a, 0x0000000000000020, 0x0000000000000022, 0x0000000000000028, 0x000000000000002a, 0x0000000000000080, 0x0000000000000082, 0x0000000000000088, 0x000000000000008a, 0x00000000000000a0, 0x00000000000000a2, 0x00000000000000a8, 0x00000000000000aa, },
/* group  9 */ {0x0000000000000000, 0x0000000000000200, 0x0000000000000800, 0x0000000000000a00, 0x0000000000002000, 0x0000000000002200, 0x0000000000002800, 0x0000000000002a00, 0x0000000000008000, 0x0000000000008200, 0x0000000000008800, 0x0000000000008a00, 0x000000000000a000, 0x000000000000a200, 0x000000000000a800, 0x000000000000aa00, },
/* group 10 */ {0x0000000000000000, 0x0000000000020000, 0x0000000000080000, 0x00000000000a0000, 0x0000000000200000, 0x0000000000220000, 0x0000000000280000, 0x00000000002a0000, 0x0000000000800000, 0x0000000000820000, 0x0000000000880000, 0x00000000008a0000, 0x0000000000a00000, 0x0000000000a20000, 0x0000000000a80000, 0x0000000000aa0000, },
/* group 11 */ {0x0000000000000000, 0x0000000002000000, 0x0000000008000000, 0x000000000a000000, 0x0000000020000000, 0x0000000022000000, 0x0000000028000000, 0x000000002a000000, 0x0000000080000000, 0x0000000082000000, 0x0000000088000000, 0x000000008a000000, 0x00000000a0000000, 0x00000000a2000000, 0x00000000a8000000, 0x00000000aa000000, },
/* group 12 */ {0x0000000000000000, 0x0000000200000000, 0x0000000800000000, 0x0000000a00000000, 0x0000002000000000, 0x0000002200000000, 0x0000002800000000, 0x0000002a00000000, 0x0000008000000000, 0x0000008200000000, 0x0000008800000000, 0x0000008a00000000, 0x000000a000000000, 0x000000a200000000, 0x000000a800000000, 0x000000aa00000000, },
/* group 13 */ {0x0000000000000000, 0x0000020000000000, 0x0000080000000000, 0x00000a0000000000, 0x0000200000000000, 0x0000220000000000, 0x0000280000000000, 0x00002a0000000000, 0x0000800000000000, 0x0000820000000000, 0x0000880000000000, 0x00008a0000000000, 0x0000a00000000000, 0x0000a20000000000, 0x0000a80000000000, 0x0000aa0000000000, },
/* group 14 */ {0x0000000000000000, 0x0002000000000000, 0x0008000000000000, 0x000a000000000000, 0x0020000000000000, 0x0022000000000000, 0x0028000000000000, 0x002a000000000000, 0x0080000000000000, 0x0082000000000000, 0x0088000000000000, 0x008a000000000000, 0x00a0000000000000, 0x00a2000000000000, 0x00a8000000000000, 0x00aa000000000000, },
/* group 15 */ {0x0000000000000000, 0x0200000000000000, 0x0800000000000000, 0x0a00000000000000, 0x2000000000000000, 0x2200000000000000, 0x2800000000000000, 0x2a00000000000000, 0x8000000000000000, 0x8200000000000000, 0x8800000000000000, 0x8a00000000000000, 0xa000000000000000, 0xa200000000000000, 0xa800000000000000, 0xaa00000000000000, },
};

static U64 memory_to_freelist_table[16][16] = {
/* group  0 */ {0x0000000000000000, 0x0000000000000001, 0x0000000100000000, 0x0000000100000001, 0x0000000000010000, 0x0000000000010001, 0x0000000100010000, 0x0000000100010001, 0x0000000200000000, 0x0000000200000001, 0x0000000300000000, 0x0000000300000001, 0x0000000200010000, 0x0000000200010001, 0x0000000300010000, 0x0000000300010001, },
/* group  1 */ {0x0000000000000000, 0x0000000000000100, 0x0000000400000000, 0x0000000400000100, 0x0000000000020000, 0x0000000000020100, 0x0000000400020000, 0x0000000400020100, 0x0000000800000000, 0x0000000800000100, 0x0000000c00000000, 0x0000000c00000100, 0x0000000800020000, 0x0000000800020100, 0x0000000c00020000, 0x0000000c00020100, },
/* group  2 */ {0x0000000000000000, 0x0000000000000010, 0x0000001000000000, 0x0000001000000010, 0x0000000000040000, 0x0000000000040010, 0x0000001000040000, 0x0000001000040010, 0x0000002000000000, 0x0000002000000010, 0x0000003000000000, 0x0000003000000010, 0x0000002000040000, 0x0000002000040010, 0x0000003000040000, 0x0000003000040010, },
/* group  3 */ {0x0000000000000000, 0x0000000000000200, 0x0000004000000000, 0x0000004000000200, 0x0000000000080000, 0x0000000000080200, 0x0000004000080000, 0x0000004000080200, 0x0000008000000000, 0x0000008000000200, 0x000000c000000000, 0x000000c000000200, 0x0000008000080000, 0x0000008000080200, 0x000000c000080000, 0x000000c000080200, },
/* group  4 */ {0x0000000000000000, 0x0000000000000004, 0x0000010000000000, 0x0000010000000004, 0x0000000000100000, 0x0000000000100004, 0x0000010000100000, 0x0000010000100004, 0x0000020000000000, 0x0000020000000004, 0x0000030000000000, 0x0000030000000004, 0x0000020000100000, 0x0000020000100004, 0x0000030000100000, 0x0000030000100004, },
/* group  5 */ {0x0000000000000000, 0x0000000000000400, 0x0000040000000000, 0x0000040000000400, 0x0000000000200000, 0x0000000000200400, 0x0000040000200000, 0x0000040000200400, 0x0000080000000000, 0x0000080000000400, 0x00000c0000000000, 0x00000c0000000400, 0x0000080000200000, 0x0000080000200400, 0x00000c0000200000, 0x00000c0000200400, },
/* group  6 */ {0x0000000000000000, 0x0000000000000020, 0x0000100000000000, 0x0000100000000020, 0x0000000000400000, 0x0000000000400020, 0x0000100000400000, 0x0000100000400020, 0x0000200000000000, 0x0000200000000020, 0x0000300000000000, 0x0000300000000020, 0x0000200000400000, 0x0000200000400020, 0x0000300000400000, 0x0000300000400020, },
/* group  7 */ {0x0000000000000000, 0x0000000000000800, 0x0000400000000000, 0x0000400000000800, 0x0000000000800000, 0x0000000000800800, 0x0000400000800000, 0x0000400000800800, 0x0000800000000000, 0x0000800000000800, 0x0000c00000000000, 0x0000c00000000800, 0x0000800000800000, 0x0000800000800800, 0x0000c00000800000, 0x0000c00000800800, },
/* group  8 */ {0x0000000000000000, 0x0000000000000002, 0x0001000000000000, 0x0001000000000002, 0x0000000001000000, 0x0000000001000002, 0x0001000001000000, 0x0001000001000002, 0x0002000000000000, 0x0002000000000002, 0x0003000000000000, 0x0003000000000002, 0x0002000001000000, 0x0002000001000002, 0x0003000001000000, 0x0003000001000002, },
/* group  9 */ {0x0000000000000000, 0x0000000000001000, 0x0004000000000000, 0x0004000000001000, 0x0000000002000000, 0x0000000002001000, 0x0004000002000000, 0x0004000002001000, 0x0008000000000000, 0x0008000000001000, 0x000c000000000000, 0x000c000000001000, 0x0008000002000000, 0x0008000002001000, 0x000c000002000000, 0x000c000002001000, },
/* group 10 */ {0x0000000000000000, 0x0000000000000040, 0x0010000000000000, 0x0010000000000040, 0x0000000004000000, 0x0000000004000040, 0x0010000004000000, 0x0010000004000040, 0x0020000000000000, 0x0020000000000040, 0x0030000000000000, 0x0030000000000040, 0x0020000004000000, 0x0020000004000040, 0x0030000004000000, 0x0030000004000040, },
/* group 11 */ {0x0000000000000000, 0x0000000000002000, 0x0040000000000000, 0x0040000000002000, 0x0000000008000000, 0x0000000008002000, 0x0040000008000000, 0x0040000008002000, 0x0080000000000000, 0x0080000000002000, 0x00c0000000000000, 0x00c0000000002000, 0x0080000008000000, 0x0080000008002000, 0x00c0000008000000, 0x00c0000008002000, },
/* group 12 */ {0x0000000000000000, 0x0000000000000008, 0x0100000000000000, 0x0100000000000008, 0x0000000010000000, 0x0000000010000008, 0x0100000010000000, 0x0100000010000008, 0x0200000000000000, 0x0200000000000008, 0x0300000000000000, 0x0300000000000008, 0x0200000010000000, 0x0200000010000008, 0x0300000010000000, 0x0300000010000008, },
/* group 13 */ {0x0000000000000000, 0x0000000000004000, 0x0400000000000000, 0x0400000000004000, 0x0000000020000000, 0x0000000020004000, 0x0400000020000000, 0x0400000020004000, 0x0800000000000000, 0x0800000000004000, 0x0c00000000000000, 0x0c00000000004000, 0x0800000020000000, 0x0800000020004000, 0x0c00000020000000, 0x0c00000020004000, },
/* group 14 */ {0x0000000000000000, 0x0000000000000080, 0x1000000000000000, 0x1000000000000080, 0x0000000040000000, 0x0000000040000080, 0x1000000040000000, 0x1000000040000080, 0x2000000000000000, 0x2000000000000080, 0x3000000000000000, 0x3000000000000080, 0x2000000040000000, 0x2000000040000080, 0x3000000040000000, 0x3000000040000080, },
/* group 15 */ {0x0000000000000000, 0x0000000000008000, 0x4000000000000000, 0x4000000000008000, 0x0000000080000000, 0x0000000080008000, 0x4000000080000000, 0x4000000080008000, 0x8000000000000000, 0x8000000000008000, 0xc000000000000000, 0xc000000000008000, 0x8000000080000000, 0x8000000080008000, 0xc000000080000000, 0xc000000080008000, },
};

static U64 _temp_freelist_to_memory(U64 value){
  U64 result = 0;
  fiz(16){
    U64 nibble = ((value >> (i * 4)) & 15);
    result |= freelist_to_memory_table[i][nibble];
  }
  return result;
}

static U64 _temp_memory_to_freelist(U64 value){
  U64 result = 0;
  fiz(16){
    U64 nibble = ((value >> (i * 4)) & 15);
    result |= memory_to_freelist_table[i][nibble];
  }
  return result;
}
#endif

#endif

static U64 _temp_freelist_index_to_memory_index(U64 index){
  U64 result = 0;
  if(index != 0){
    U64 high_bit = index_of_high_bit_u64(index); // nocheckin verify this step
    U64 low = index - (LIT_U64(1) << high_bit);

    result  = (LIT_U64(64) >> (high_bit + 1));
    result += (low << (LIT_U64(6) - high_bit));
  }
  return result;
}

static void* _temp_alloc_on_page(Allocator *allocator, TempPage *page, size_t size){
  void *result = NULL;

  assert(allocator->kind == ALLOCATOR_KIND_temporary);

  // TODO: An optimization idea: Do
  //   mask = (mask << 1) | mask
  // `alloc_units` number of times. This can be done in ~6 steps.
  // Then it suffices to do one find first set bit operation.
  // We could also do this approximately if doing it exactly is slow.

  U64 unit_count = (size + TEMP_ALLOC_UNIT - 1) / TEMP_ALLOC_UNIT;
  assert(unit_count <= 63);
  U64 linear_mask = (LIT_U64(1) << unit_count) - 1;

  U64 free_list = _temp_memory_to_freelist(page->free_list_memory_order);
  while(free_list != (U64)(-1)){
    U64 index = index_of_low_bit_u64(~free_list);
    U64 memory_index = _temp_freelist_index_to_memory_index(index);
    if(memory_index + unit_count <= 64 && ((page->free_list_memory_order >> memory_index) & linear_mask) == 0){
      // That position was large enough. Use it!
      page->free_list_memory_order |= (linear_mask << memory_index);
      result = page->data + TEMP_ALLOC_UNIT * memory_index;
      break;
    }else{
      free_list |= (LIT_U64(1) << index); // TODO: This can be done more efficiently by clearing up to the next bit.
    }
  }

  return result;
}

static void* _temp_alloc(Allocator *allocator, size_t size){
  void *result = NULL;
  assert(size < BIG_ALLOC_SIZE);
  assert(allocator->kind == ALLOCATOR_KIND_temporary);

  for(AllocatorPageHeader *header = allocator->temp_sentinel.next; header != &allocator->temp_sentinel; header = header->next){
    TempPage *page = (TempPage*)header;
    result = _temp_alloc_on_page(allocator, page, size);
    if(result) break;
  }

  if(!result){
    TempPage *page = (TempPage*)_allocator_alloc_pages(allocator, &allocator->temp_sentinel, 1);
    page->free_list_memory_order = LIT_U64(0x8000000000000000);

    result = _temp_alloc_on_page(allocator, page, size);
    assert(result);
  }

  return result;
}

static void _temp_free(Allocator *allocator, void *ptr, size_t size){
  assert(allocator->kind == ALLOCATOR_KIND_temporary);
  assert(size < BIG_ALLOC_SIZE);
  assert(ptr != allocator_get_stub(allocator));

  if(ptr){
    TempPage *page = (TempPage*)get_allocator_page(ptr);

    U64 index = (((U64)ptr & (U64)(OS_PAGE_SIZE - 1)) >> TEMP_ALLOC_UNIT_LOG2) - 1; // Subtract one to compensate for the TempPage header
    U64 unit_count = (size + TEMP_ALLOC_UNIT - 1) >> TEMP_ALLOC_UNIT_LOG2;

    U64 mask = ((LIT_U64(1) << unit_count) - 1) << index;
    page->free_list_memory_order &= ~mask;
  }
}
static void _temp_realloc(Allocator *allocator, void **_ptr, size_t old_size, size_t new_size){
  assert(allocator->kind == ALLOCATOR_KIND_temporary);

  assert(old_size <= new_size);
  void *ptr = *_ptr;
  assert(ptr != allocator_get_stub(allocator));

  assert(old_size < BIG_ALLOC_SIZE);
  assert(new_size < BIG_ALLOC_SIZE);

  if(!ptr){
    assert(old_size == 0);
    ptr = _temp_alloc(allocator, new_size);
  }else{ // Small alloc
    TempPage *page = (TempPage*)get_allocator_page(ptr);

    // TODO: Move some of this logic to the expand operation!

    U64 index = (((uintptr_t)ptr & (OS_PAGE_SIZE - 1)) >> TEMP_ALLOC_UNIT_LOG2) - 1;
    U64 old_unit_count = (old_size + TEMP_ALLOC_UNIT - 1) >> TEMP_ALLOC_UNIT_LOG2;
    U64 new_unit_count = (new_size + TEMP_ALLOC_UNIT - 1) >> TEMP_ALLOC_UNIT_LOG2;

    U64 small_mask = ((LIT_U64(1) << old_unit_count) - 1) << index;
    U64 big_mask = ((LIT_U64(1) << new_unit_count) - 1) << index;
    U64 complementary_mask = big_mask ^ small_mask;

    if(index + new_unit_count < 64 && (page->free_list_memory_order & complementary_mask) == 0){
      // There is room for us to expand
      page->free_list_memory_order |= big_mask;
    }else{
      // There is not a sufficient amount of room left. We need to move the allocation someplace else
      void *new_alloc = _temp_alloc(allocator, new_size);
      memcpy(new_alloc, ptr, old_size);
      _temp_free(allocator, ptr, old_size);
      ptr = new_alloc;
    }
  }

  *_ptr = ptr;
}

//~ Alloc/Free

static void* allocator_get_stub(Allocator *allocator){
  // This needs to be aligned to the maximum alignment because we want to be able to assert that all our allocations are properly aligned.
  // (Even though this allocation not supposed to be used.)
  return allocator_page_header_data(allocator->stub) + (ALLOCATOR_MAX_ALIGN - sizeof(AllocatorPageHeader));
}

// Returns true if the allocation could be resized, false otherwise
static bool allocator_expand(void *ptr, size_t old_size, size_t new_size){
  assert(new_size > old_size);

  Allocator *allocator = get_allocator(ptr);

  bool result = false;
  if(ptr == allocator_get_stub(allocator)){
    // You can of course not expand the stub allocation.
    assert(old_size == 0);
  }else if(old_size < BIG_ALLOC_SIZE){
    switch(allocator->kind){
      case ALLOCATOR_KIND_heap:{
        // TODO: Make stuff happen here
      }break;

      case ALLOCATOR_KIND_temporary:{
        // TODO: And also make stuff happen here!
      }break;

      default:{
        panic("Unknown allocator kind!");
      }break;
    }
  }else{

  }
  return result;
}

static void allocator_free(void *ptr, size_t size){
  if(!ptr){
    assert(size == 0);
    // Do nothing!
  }else{
    Allocator *allocator = get_allocator(ptr);

    if(ptr == allocator_get_stub(allocator)){
      assert(size == 0);
      // Do nothing!
    }else if(size < BIG_ALLOC_SIZE){
      switch(allocator->kind){
        case ALLOCATOR_KIND_heap:{
          _heap_free(allocator, ptr, size);
        }break;

        case ALLOCATOR_KIND_temporary:{
          _temp_free(allocator, ptr, size);
        }break;

        default:{
          panic("Unknown allocator kind!");
        }break;
      }
    }else{
      BigAllocHeader *page = (BigAllocHeader*)get_allocator_page(ptr);
      _allocator_free_pages_unlink(allocator, &page->page_header);
    }
  }
}

static void allocator_realloc_noclear(void **_ptr, size_t old_size, size_t new_size, size_t align){
  assert(align != 0 && "Use align == 1 for no alignment requirements");
  assert(IS_POWER_OF_TWO(align));
  assert(((uintptr_t)*_ptr & (uintptr_t)(align - 1)) == 0 && "Existing alloc must be properly aligned!");
  assert(align <= ALLOCATOR_MAX_ALIGN);

  void *old_ptr = *_ptr;
  assert(old_ptr);
  Allocator *allocator = get_allocator(old_ptr);

  if(old_ptr != allocator_get_stub(allocator) && old_size < BIG_ALLOC_SIZE && new_size < BIG_ALLOC_SIZE && allocator->kind == ALLOCATOR_KIND_temporary){
    assert(align <= TEMP_ALLOC_UNIT);
    _temp_realloc(allocator, _ptr, old_size, new_size);
  }else{
    if(new_size < BIG_ALLOC_SIZE){
      switch(allocator->kind){
        case ALLOCATOR_KIND_heap:{
          *_ptr = _heap_alloc(allocator, new_size, align);
        }break;

        case ALLOCATOR_KIND_temporary:{
          assert(align <= TEMP_ALLOC_UNIT);
          *_ptr = _temp_alloc(allocator, new_size);
        }break;

        default:{
          panic("Unknown allocator kind!");
        }break;
      }
    }else{
      *_ptr = _big_alloc_alloc(allocator, new_size, align);
    }
    void *new_ptr = *_ptr;

    memcpy(new_ptr, old_ptr, MINIMUM(old_size, new_size));
    allocator_free(old_ptr, old_size);
  }
}

static void* allocator_alloc_noclear(Allocator *allocator, size_t size, size_t align){
  void *result = allocator_get_stub(allocator);
  allocator_realloc_noclear(&result, 0, size, align);
  return result;
}
static void* allocator_alloc_clear(Allocator *allocator, size_t size, size_t align){
  void *result = allocator_alloc_noclear(allocator, size, align);
  memset(result, 0, size);
  return result;
}
#define allocator_alloc_item_noclear(_allocator_, _Type_) ( (_Type_*) allocator_alloc_noclear((_allocator_), sizeof(_Type_), __alignof__(_Type_)) )
#define allocator_alloc_item_clear(_allocator_, _Type_)   ( (_Type_*) allocator_alloc_clear  ((_allocator_), sizeof(_Type_), __alignof__(_Type_)) )

static String allocator_get_string_stub(Allocator *allocator){
  String result = {0};
  result.data = (U8*)allocator_get_stub(allocator);
  return result;
}
static String allocator_alloc_string(Allocator *allocator, String string){
  String result = {0};

  result.data = (U8*)allocator_alloc_noclear(allocator, string.size, 1);
  memcpy(result.data, string.data, string.size);
  result.size = string.size;

  return result;
}
static String allocator_alloc_string_uninitialized(Allocator *allocator, I64 size){
  String result = {0};

  result.data = (U8*)allocator_alloc_noclear(allocator, size, 1);
  result.size = size;

  return result;
}
static void allocator_realloc_string(String *dst, String src){
  Allocator *allocator = get_allocator(dst->data);
  allocator_free(dst->data, dst->size);
  clear_item(dst);
  dst->data = (U8*)allocator_alloc_noclear(allocator, src.size, 1);
  dst->size = src.size;
  memcpy(dst->data, src.data, src.size);
}
static void allocator_free_string(String *string){
  allocator_free(string->data, string->size);
  clear_item(string);
}

//~ Push allocation (Permanent allocations only freed when the allocator is destroyed)

static PushChunkHeader* _allocator_current_push_chunk(Allocator *allocator){
  return (PushChunkHeader*)allocator->push_sentinel.page_header.next;
}
static U8* _allocator_current_push_chunk_data(Allocator *allocator){
  PushChunkHeader *header = _allocator_current_push_chunk(allocator);
  return push_chunk_data(header);
}
static size_t _allocator_current_push_chunk_size(Allocator *allocator){
  PushChunkHeader *header = _allocator_current_push_chunk(allocator);
  return header->page_header.page_count * OS_PAGE_SIZE - sizeof(PushChunkHeader);
}

static void* allocator_push_memory(Allocator *allocator, size_t element_size, size_t element_count, size_t align, bool clear){
  // TODO(hanna - 2022-12-06): Larger allocations should go through a separate path!
  // TODO: Should we perhaps go into the out of memory path when these are this large?
  assert(element_size < UINT32_MAX && "Very large element size");
  assert(element_count < UINT32_MAX && "Very large element count");
  size_t size = element_size * element_count;

  assert(allocator);
  assert(align <= 64 && "Very large alignment");
  assert(IS_POWER_OF_TWO(align));

  allocator->push_cursor = ((allocator->push_cursor + align - 1) & ~(align - 1));

  if(!allocator->push_cursor || allocator->push_cursor + size > (uintptr_t)_allocator_current_push_chunk_data(allocator) + _allocator_current_push_chunk_size(allocator)){
    size_t chunk_size = MAXIMUM(KILOBYTES(16), size + sizeof(PushChunkHeader) + align - 1);
    chunk_size = (chunk_size + (size_t)(OS_PAGE_SIZE - 1)) & ~(size_t)(OS_PAGE_SIZE - 1);
    U64 chunk_page_count = chunk_size >> OS_PAGE_SIZE_LOG2;

    PushChunkHeader *chunk = (PushChunkHeader*)_allocator_alloc_pages(allocator, &allocator->push_sentinel.page_header, chunk_page_count);
    assert(_allocator_current_push_chunk(allocator) == chunk);
    allocator->push_cursor = (uintptr_t)push_chunk_data(chunk);
    allocator->push_cursor = ((allocator->push_cursor + align - 1) & ~(align - 1));
  }

  assert((allocator->push_cursor & (align - 1)) == 0);
  assert(allocator->push_cursor + size <= (uintptr_t)_allocator_current_push_chunk_data(allocator) + _allocator_current_push_chunk_size(allocator));

  void *result = (void*)allocator->push_cursor;
  allocator->push_cursor += size;

  if(clear){
    memset(result, 0, size);
  }

  return result;
}

#define allocator_push_items_noclear(_allocator_, _Type_, _n_) ( (_Type_*)allocator_push_memory((_allocator_), sizeof(_Type_), (size_t)(_n_), ALIGNOF_TYPE(_Type_), false) )
#define allocator_push_items_clear(_allocator_, _Type_, _n_) ( (_Type_*)allocator_push_memory((_allocator_), sizeof(_Type_), (size_t)(_n_), ALIGNOF_TYPE(_Type_), true) )
#define allocator_push_item_noclear(_allocator_, _Type_) allocator_push_items_noclear((_allocator_), _Type_, 1)
#define allocator_push_item_clear(_allocator_, _Type_) allocator_push_items_clear((_allocator_), _Type_, 1)

static char* allocator_push_string_as_cstring(Allocator *allocator, String string){
  char *result = allocator_push_items_noclear(allocator, char, string.size + 1);
  memcpy(result, string.data, string.size);
  result[string.size] = '\0';
  return result;
}

static U8* allocator_push_data(Allocator *allocator, U8 *data, size_t size){
  U8 *result = (U8*)allocator_push_items_noclear(allocator, U8, size);
  memcpy(result, data, size);
  return result;
}
static String allocator_push_string(Allocator *allocator, String string){
  String result = {0};
  if(string.size > 0){
    U8 *data = (U8*) allocator_push_items_noclear(allocator, U8, string.size);
    memcpy(data, string.data, string.size);
    result.data = data;
    result.size = string.size;
  }
  return result;
}

static String allocator_push_string_repeat(Allocator *allocator, String string, int count){
  String result = {0};
  result.data = (U8*)allocator_push_items_noclear(allocator, U8, string.size * count);
  fiz(count){
    memcpy(result.data + result.size, string.data, string.size);
    result.size += string.size;
  }
  return result;
}


static String allocator_push_vprintf(Allocator *allocator, const char *format, va_list list1){
  va_list list2;
  va_copy(list2, list1);
  int size = stbsp_vsnprintf(NULL, 0, format, list1);
  String result = {0};
  result.data = allocator_push_items_noclear(allocator, U8, size + 1);
  result.size = (U32)size;
  stbsp_vsnprintf((char*)result.data, result.size + 1, format, list2);
  va_end(list2);
  return result;
}
static String allocator_push_printf(Allocator *allocator, const char *format, ...){
  va_list args;
  va_start(args, format);
  String result = allocator_push_vprintf(allocator, format, args);
  va_end(args);
  return result;
}

static char* allocator_push_cstring(Allocator *allocator, String string){
  char *result = allocator_push_items_noclear(allocator, char, string.size + 1);
  memcpy(result, string.data, string.size);
  result[string.size] = '\0';
  return result;
}

//
// OTHER UTILITY CODE
//

//~ Rolling hash

// NOTE(hanna - 2021-05-08): Some random prime number from the internet. I have no idea if it is good or
// not.
#define ROLLING_HASH_COEFFICIENT (4611686018427387631ULL)
typedef struct RollingHash RollingHash;
struct RollingHash{
  U8 *buffer;
  size_t buffer_size;
  U32 window_size;
  U64 coefficient_pow_window_size;
  // STATE
  U64 hash;
  I64 index;
};

static U64 rolling_hash_compute_hash(U8 *buffer, size_t buffer_size){
  U64 result = 0;
  fiz(buffer_size){
    result = result * ROLLING_HASH_COEFFICIENT + buffer[i];
  }
  return result;
}
static RollingHash rolling_hash_create(U8 *buffer, size_t buffer_size, U32 window_size){
  RollingHash result = {0};
  result.buffer = buffer;
  result.buffer_size = buffer_size;
  result.window_size = window_size;

  if(window_size <= buffer_size){
    result.coefficient_pow_window_size = 1;
    fiz(window_size){
      result.hash = result.hash * ROLLING_HASH_COEFFICIENT + buffer[i];
      result.coefficient_pow_window_size *= ROLLING_HASH_COEFFICIENT;
    }
  }
  return result;
}
static bool rolling_hash_is_valid(RollingHash *rh){
  return rh->index + rh->window_size <= rh->buffer_size;
}
static void rolling_hash_advance(RollingHash *rh){
  if(rh->index + rh->window_size < rh->buffer_size){
    rh->hash *= ROLLING_HASH_COEFFICIENT;
    rh->hash += rh->buffer[rh->index + rh->window_size];
    rh->hash -= rh->buffer[rh->index] * rh->coefficient_pow_window_size;
  }
  rh->index += 1;
}

//~ CSV loading code

#if 0

typedef struct CSVFile CSVFile;
struct CSVFile{
  Allocator *push_allocator;

  String file_content;
  U64 at;
  char separator_ascii_character;

  U32 column_count;
  String *column_names;

  U32 row_count;

  U32 one_past_current_row_number;
  String *current_row; // array of `column_count` values
};

static String read_line(String file_content, U64 *_at){
  assert(_at);
  U64 at = *_at;
  assert(at < file_content.size);

  U64 line_begin = at;
  while(at < file_content.size && file_content.data[at] != '\n'){
    at += 1;
  }
  U64 line_end = at;
  if(at < file_content.size && file_content.data[at] == '\n'){
    at += 1;
  }

  String result = substring(file_content, line_begin, line_end);
  *_at = at;
  return result;
}


static bool csv_file_read_row(CSVFile *file, String *_error_message){
  bool result = false;
  String error_message = {0};
  if(_error_message){
    error_message = *_error_message;
  }

  if(file->at < file->file_content.size){
    result = true;
    file->one_past_current_row_number += 1;
    String line = read_line(file->file_content, &file->at);

    U64 cursor = 0;
    U64 num_lexed_fields = 0;

    while(cursor <= line.size){
      String token = {0};

      if(cursor < line.size && line.data[cursor] == '"'){
        cursor += 1;
        U64 token_begin = cursor;
        while(cursor < line.size && line.data[cursor] != '"'){
          cursor += 1;
        }

        U64 token_end = cursor;
        if(cursor == line.size){
          result = false;
          str_printf(&error_message, "Unterminated quote on row %d.", file->one_past_current_row_number - 1);
          goto failure;
        }
        cursor += 1; // eat quote

        if(cursor + 1 < line.size && line.data[cursor + 1] != file->separator_ascii_character){
          result = false;
          str_printf(&error_message, "Expected separator character `%c` after double-quoted field on row %d.", file->separator_ascii_character, file->one_past_current_row_number - 1);
          goto failure;
        }
        cursor += 1; // skip separator character or skip past the end of the line

        token = substring(line, token_begin, token_end);
      }else{
        U64 token_begin = cursor;
        while(cursor < line.size && line.data[cursor] != file->separator_ascii_character){
          cursor += 1;
        }

        U64 token_end = cursor;
        cursor += 1;
        token = substring(line, token_begin, token_end);
      }

      if(num_lexed_fields < file->column_count){
        file->current_row[num_lexed_fields] = token;
        num_lexed_fields += 1;
      }else{
        result = false;
        str_printf(&error_message, "Too many entries on row %d.", file->one_past_current_row_number - 1);
        goto failure;
      }
    }failure:;

    if(result && num_lexed_fields < file->column_count){
      result = false;
      str_printf(&error_message, "Too few entries on row %d.", file->one_past_current_row_number - 1);
    }
  }

  if(_error_message){
    *_error_message = error_message;
  }

  return result;
}

static CSVFile csv_file_create(MemoryArena *arena, String file_content, char separator_ascii_character, U32 column_count, String *column_names, String *_error_message){
  String error_message = _error_message ? *_error_message : (String){0};

  CSVFile result = {0};
  result.arena = arena;
  result.file_content = file_content;
  result.separator_ascii_character = separator_ascii_character;
  result.column_count = column_count;
  result.column_names = column_names;

  result.current_row = arena_push_items_noclear(arena, String, column_count);

  if(csv_file_read_row(&result, &error_message)){
    fiz(result.column_count){
      String field = result.current_row[i];

      if(!string_equals(field, column_names[i])){
        str_printf(&error_message, "Header of file doesn't match given strings (expected `%*.s`, found `%*.s`)", column_names[i], field);
        break;
      }
    }

    if(error_message.size == 0){
      U64 at = result.at;

      while(at < file_content.size){
        read_line(file_content, &at);
        result.row_count += 1;
      }
    }
  }else{
    result = (CSVFile){0};
  }

  if(_error_message){
    *_error_message = error_message;
  }

  return result;
}


/*
NOTE(hanna - 2020-11-27):
The intended usage here is to to have an outer loop over each row (AKA record) of the CSV file and in this loop you use a pair
of these to process each of the fields to make sure that the fields are being processed in the right order. In this outer loop
you have a `field_cursor` which will be read and then incremented by these macros to process the fields one by one.

In code, this would look somewhat like this:

for(I64 row_cursor = 0; csv_file_read_row(&csv, &error_message); row_cursor += 1){
  I64 field_cursor = 0;

  CSV_BEGIN_FIELD("name"){
    names[row_cursor] = arena_push_string(arena, field_string);
  } CSV_END_FIELD();

  CSV_BEGIN_FIELD("cost"){
    I64 value;
    if(parse_i64(field_string, &value, 10)){
      costs[row_cursor] = value;
    }else{
      failure = true;
    }
  } CSV_END_FIELD();
}

*/
#define CSV_BEGIN_FIELD(_csv_, _name_) do{ assert((U8*)(_name_) == (_csv_)->column_names[field_cursor].data); String field_string = csv.current_row[field_cursor]; if(1)
#define CSV_END_FIELD(_csv_) do{ field_cursor += 1; } while(false); } while(false)

#define CSV_PARSE_F64_FIELD(_csv_, _name_, _array_, _error_message_) \
  do{ \
    CSV_BEGIN_FIELD(_csv_, _name_); \
      F64 value; \
      if(parse_base10_string_as_f64(field_string, &value)){ \
        (_array_)[row_cursor] = value;\
      }else{ \
        str_printf((_error_message_), \
                         "Unable to parse field `%s` on row %d. Unable to parse field value `%.*s` as base 10 "\
                         "decimal number.", (_name_), (_csv_)->one_past_current_row_number - 1, \
                         StrFormatArg(field_string)); \
      }\
    CSV_END_FIELD(_csv_); \
  }while(false)
#endif

//~ Parsing utility

static bool parse_i64(String string, I64 *_output, I64 base, bool allow_negative){
  bool result = false;
  I64 output = 0;

  if(string.size > 0){
    I64 i = 0;
    bool negative = false;
    if(string.data[i] == '-' && allow_negative){
      i += 1;
      negative = true;
    }

    // TODO: Overflow protection?

    result = true;
    for(; i < string.size; i+= 1){
      int digit = -1;
      if('0' <= string.data[i] && string.data[i] <= '9'){
        digit = string.data[i] - '0';
      }else if('a' <= string.data[i] && string.data[i] <= 'f'){
        digit = string.data[i] + 10 - 'a';
      }else if('A' <= string.data[i] && string.data[i] <= 'F'){
        digit = string.data[i] + 10 - 'A';
      }

      if(digit == -1 || digit >= base){
        result = false;
        output = 0;
        break;
      }

      output = output * base + digit;
    }

    if(negative){
      output = -output;
    }
  }

  *_output = output;
  return result;
}

static bool parse_base10_string_as_f64(String string, F64 *_output){
  bool result = false;

  F64 output = 0;
  // TODO(hanna - 2020-11-21): Implement our own base10 string -> IEEE754 double precision floating point routine which handles bad input correctly.

  char c_string[256];

  if(string.size < sizeof(c_string)){
    result = true;

    memcpy(c_string, string.data, string.size);
    c_string[string.size] = '\0';

    errno = 0;
    char *end_pointer;
    output = strtod(c_string, &end_pointer);

    if(errno && end_pointer != c_string + string.size){
      result = false;
    }
  }

  if(_output){
    *_output = output;
  }

  return result;
}

//~ Basic lexer

// A very simplistic lexer built for very simplistic syntax highlighting.
typedef struct BasicLexer BasicLexer;
struct BasicLexer{
  String content;
  I64 cursor;

#define BASIC_TOKEN_KIND_whitespace         1
#define BASIC_TOKEN_KIND_identifier         2
#define BASIC_TOKEN_KIND_number             3
#define BASIC_TOKEN_KIND_string             4
#define BASIC_TOKEN_KIND_codepoint          5
#define BASIC_TOKEN_KIND_comment            6
  U32 token_kind;
  String token;
};
static bool _basic_lexer_is_ascii_letter(U32 codepoint){
  return ('a' <= codepoint && codepoint <= 'z') || ('A' <= codepoint && codepoint <= 'Z');
}
static bool _basic_lexer_is_ascii_digit(U32 codepoint){
  return ('0' <= codepoint && codepoint <= '9');
}
static bool _basic_lexer_is_ascii_horz_whitespace(U32 codepoint){
  return (codepoint == '\t' || codepoint == ' ');
}
static bool basic_lexer_next_token(BasicLexer *l){
  bool result = true;
  I64 begin = l->cursor;
  if(l->cursor >= l->content.size){ // Nothing left!
    result = false;
  }else if(_basic_lexer_is_ascii_horz_whitespace(l->content.data[l->cursor])){
    l->cursor += 1;
    l->token_kind = BASIC_TOKEN_KIND_whitespace;
    while(l->cursor < l->content.size && _basic_lexer_is_ascii_horz_whitespace(l->content.data[l->cursor])){
      l->cursor += 1;
    }
  }else if((_basic_lexer_is_ascii_letter(l->content.data[l->cursor]) || l->content.data[l->cursor] == '_')){ // Word!
    l->cursor += 1;
    l->token_kind = BASIC_TOKEN_KIND_identifier;
    while(l->cursor < l->content.size && (_basic_lexer_is_ascii_letter(l->content.data[l->cursor]) || l->content.data[l->cursor] == '_' || _basic_lexer_is_ascii_digit(l->content.data[l->cursor]))){
      l->cursor += 1;
    }
  }else if(_basic_lexer_is_ascii_digit(l->content.data[l->cursor])){ // Number!!
    l->cursor += 1;
    l->token_kind = BASIC_TOKEN_KIND_number;
    while(l->cursor < l->content.size && _basic_lexer_is_ascii_digit(l->content.data[l->cursor])){
      l->cursor += 1;
    }
  }else if(l->content.data[l->cursor] == '"'){
    l->cursor += 1;
    l->token_kind = BASIC_TOKEN_KIND_string;
    while(l->cursor < l->content.size && l->content.data[l->cursor] != '\n' && l->content.data[l->cursor] != '"'){
      if(l->content.data[l->cursor] == '\\'){
        l->cursor += 2;
      }else{
        l->cursor += 1;
      }
    }
    l->cursor += 1;
  }else if(l->cursor + 2 <= l->content.size && l->content.data[l->cursor + 0] == '/' && l->content.data[l->cursor + 1] == '/'){
    // A line comment
    l->token_kind = BASIC_TOKEN_KIND_comment;
    while(l->cursor < l->content.size && l->content.data[l->cursor] != '\n'){
      l->cursor += 1;
    }
    l->cursor += 1;
  }else{ // A stray codepoint
    l->token_kind = BASIC_TOKEN_KIND_codepoint;
    next_codepoint(l->content, &l->cursor);
  }
  I64 end = l->cursor;

  l->token = substring(l->content, begin, end);

  return result;
}

//~ Data vector utility

#if 0

static F64* f64vector_compute_squared_differances(MemoryArena *arena, I64 element_count, F64 *a, F64 *b){
  F64 *result = arena_push_items_noclear(arena, F64, element_count);
  fiz(element_count){
    result[i] = SQUARE(a[i] - b[i]);
  }
  return result;
}
static F64 f64vector_compute_sum(I64 element_count, F64 *values){
  F64 result = 0;
  fiz(element_count){
    result += values[i];
  }
  return result;
}
#endif


//~ Floating point

static F32 f32_fractional_part(F32 x){
  F32 i;
  return modff(x, &i);
}

static void assert_f32_is_not_fishy(F32 x){
  assert(x == x);
  assert(x != INFINITY);
  assert(x != -INFINITY);
}

static F32 f32_mix(F32 a, F32 factor, F32 b){
  return a * (1 - factor) + b * factor;
}

static I32 fast_floor_f32_to_i32(F32 value){ // TODO: Think through this
  return (I32)value - (I32)(value < 0);
}

static bool f32_approx_equals(F32 a, F32 b, F32 epsilon){
  bool result = false;
  if(-epsilon < a - b && a - b < epsilon){
    result = true;
  }
  return result;
}

static F32 f32_absolute(F32 value){
  if(value < 0) return -value;
  return value;
}

static F32 f32_clamp(F32 lower, F32 value, F32 upper){
  F32 result = value;
  assert(lower <= upper);
  if(result < lower){
    result = lower;
  }else if(result > upper){
    result = upper;
  }
  return result;
}

static F32 f32_clamp01(F32 value){
  return f32_clamp(0, value, 1);
}

static F32 f32_min2(F32 a, F32 b){
  return MINIMUM(a, b);
}
static F32 f32_min3(F32 a, F32 b, F32 c){
  F32 ab = MINIMUM(a, b);
  return MINIMUM(ab, c);
}
static F32 f32_min4(F32 a, F32 b, F32 c, F32 d){
  F32 ab = MINIMUM(a, b);
  F32 cd = MINIMUM(c, d);
  return MINIMUM(ab, cd);
}

static F32 f32_max2(F32 a, F32 b){
  return MAXIMUM(a, b);
}
static F32 f32_max3(F32 a, F32 b, F32 c){
  F32 ab = MAXIMUM(a, b);
  return MAXIMUM(ab, c);
}
static F32 f32_max4(F32 a, F32 b, F32 c, F32 d){
  F32 ab = MAXIMUM(a, b);
  F32 cd = MAXIMUM(c, d);
  return MAXIMUM(ab, cd);
}

static F32 f32_sign(F32 value){
  if(value > 0) return +1;
  if(value < 0) return -1;
  return 0;
}

#define F32_INFINITY INFINITY
#define F64_INFINITY INFINITY
#define F32_NAN (0.f / 0.f)
#define F64_NAN (0.0 / 0.0)

//~ Swapping

static void f32_swap(F32 *a, F32 *b){
  F32 tmp = *a;
  *a = *b;
  *b = tmp;
}
static void u32_swap(U32 *a, U32 *b){
  U32 tmp = *a;
  *a = *b;
  *b = tmp;
}
static void i64_swap(I64 *a, I64 *b){
  I64 tmp = *a;
  *a = *b;
  *b = tmp;
}
static void u64_swap(U64 *a, U64 *b){
  U64 tmp = *a;
  *a = *b;
  *b = tmp;
}
static void u8_swap(U8 *a, U8 *b){
  U8 tmp = *a;
  *a = *b;
  *b = tmp;
}

//~ Comparison

static int i64_compare(I64 a, I64 b){
  int result;
  if(a == b){
    result = 0;
  }else if(a > b){
    result = 1;
  }else{
    result = -1;
  }
  return result;
}
static int string_compare(String a, String b){
  int result = i64_compare(a.size, b.size);
  if(result == 0){
    result = memcmp(a.data, b.data, a.size);
  }
  return result;
}


//~ Bitset

typedef struct Bitset Bitset;
struct Bitset{
  U64 *bits;
  U64 num_bits;
};
static Bitset bitset_create(Allocator *allocator, U64 num_bits){
  Bitset result = {0};
  result.num_bits = num_bits;
  result.bits = allocator_push_items_clear(allocator, U64, (num_bits + 0x3f) >> 6);
  return result;
}
static bool bitset_get(Bitset *bitset, U64 index){
  assert(index < bitset->num_bits);
  bool result = false;
  U64 high_index = index >> 6;
  U64 low_index = index & 0x3f;
  if(bitset->bits[high_index] & ((U64)1 << low_index)){
    result = true;
  }
  return result;
}
static void bitset_set(Bitset *bitset, U64 index, bool value){
  assert(index < bitset->num_bits);
  U64 high_index = index >> 6;
  U64 low_index = index & 0x3f;
  bitset->bits[high_index] &= ~((U64)1 << low_index);
  bitset->bits[high_index] |= (U64)(!!value) << low_index;
}

//~ Bits

static U32 u32_bitswap(U32 value, U32 bit_a, U32 bit_b){
  U32 result = value;
  result &= ~((1U << bit_a) | (1U << bit_b));
  result |= ((value >> bit_a) & 1U) << bit_b;
  result |= ((value >> bit_b) & 1U) << bit_a;
  return result;
}

//~ Bitwise conversion

static U64 f64_bitwise_as_u64(F64 value){
  union{
    U64 u64;
    F64 f64;
  } conversion;
  conversion.f64 = value;
  return conversion.u64;
}
static F64 u64_bitwise_as_f64(U64 value){
  union{
    U64 u64;
    F64 f64;
  } conversion;
  conversion.u64 = value;
  return conversion.f64;
}

static U32 f32_bitwise_as_u32(F32 value){
  union{
    U32 u32;
    F32 f32;
  } conversion;
  conversion.f32 = value;
  return conversion.u32;
}
static F32 u32_bitwise_as_f32(U32 value){
  union{
    U32 u32;
    F32 f32;
  } conversion;
  conversion.u32 = value;
  return conversion.f32;
}

// TODO: WTF? Are these really neccesary?
static U64 i64_bitwise_as_u64(I64 value){
  union{
    U64 u64;
    I64 i64;
  } conversion;
  conversion.i64 = value;
  return conversion.u64;
}
static I64 u64_bitwise_as_i64(U64 value){
  union{
    U64 u64;
    I64 i64;
  } conversion;
  conversion.u64 = value;
  return conversion.i64;
}

//
// NOTE: Vector and matrix math
//

typedef struct V2{ F32 x, y; } V2;
typedef struct V2i{ I32 x, y; } V2i;
typedef union V3{ struct{ F32 x, y, z; }; struct{ F32 r, g, b; }; F32 e[3]; } V3;
typedef struct V3i{ I32 x, y, z; } V3i;
typedef union V4{ struct{ F32 x, y, z, w; }; struct{ F32 r, g, b, a; }; struct{ V3 rgb; }; } V4;
typedef struct Mat2x2{ F32 e[2][2]; } Mat2x2;
#define MAT2X2_IDENTITY ((Mat2x2){ .e[0][0] = 1, .e[1][1] = 1 })

typedef struct Mat4x4{ F32 e[4][4]; } Mat4x4;
// NOTE: We unfortunately need this crazyness to be able to properly compile as C++
#define MAT4x4_IDENTITY ( mat4x4_identity() )
static Mat4x4 mat4x4_identity(){
  Mat4x4 result = {0};
  result.e[0][0] = 1;
  result.e[1][1] = 1;
  result.e[2][2] = 1;
  result.e[3][3] = 1;
  return result;
}

typedef struct Mat3x3{ F32 e[3][3]; } Mat3x3;
static Mat3x3 mat3x3_identity(){
  Mat3x3 result = {0};
  result.e[0][0] = 1;
  result.e[1][1] = 1;
  result.e[2][2] = 1;
  return result;
}

//~ V2

static V2 vec2(F32 x, F32 y){ V2 result = { x, y }; return result; }
static V2 v2_scalar_mul(V2 a, F32 scalar){ return vec2(a.x * scalar, a.y * scalar); }
static V2 v2_add(V2 a, V2 b){ return vec2(a.x + b.x, a.y + b.y); }
static V2 v2_sub(V2 a, V2 b){ return vec2(a.x - b.x, a.y - b.y); }
static F32 v2_dot(V2 a, V2 b){ return a.x * b.x + a.y * b.y; }
static V2 v2_mix(V2 a, F32 t, V2 b){ return v2_add( v2_scalar_mul(a, 1 - t), v2_scalar_mul(b, t) ); }
static V2 v2_negate(V2 a){ return vec2(-a.x, -a.y); }
static V2 v2_componentwise_div(V2 a, V2 b){ return vec2(a.x / b.x, a.y / b.y); }
static V2 v2_componentwise_mul(V2 a, V2 b){ return vec2(a.x * b.x, a.y * b.y); }
static V2 v2_hadamard(V2 a, V2 b){ return vec2(a.x * b.x, a.y * b.y); }
static F32 v2_distance_sq(V2 a, V2 b){ return SQUARE(a.x - b.x) + SQUARE(a.y - b.y); }
static F32 v2_length_sq(V2 v){ return SQUARE(v.x) + SQUARE(v.y); }
static V2 v2_normalize(V2 v){
  F32 inv_length = 1.f / sqrtf(SQUARE(v.x) + SQUARE(v.y));
  return vec2(v.x * inv_length, v.y * inv_length);
}
static F32 v2_cross(V2 a, V2 b){ return a.x * b.y - a.y * b.x; }
static bool v2_bitwise_equal(V2 a, V2 b){ return (a.x == b.x && a.y == b.y); }
static V2 v2_perpendicular(V2 a){ return vec2(-a.y, a.x); }

#ifdef __cplusplus
static V2 operator+(V2 a, V2 b){ return vec2(a.x + b.x, a.y + b.y); }
static V2 operator+=(V2 &a, V2 b){ return a = a + b; }
static V2 operator-(V2 a, V2 b){ return vec2(a.x - b.x, a.y - b.y); }
static V2 operator-=(V2 &a, V2 b){ return a = a - b; }
static V2 operator*(V2 a, V2 b){ return vec2(a.x * b.x, a.y * b.y); } // Hadamard
static V2 operator/(V2 a, V2 b){ return vec2(a.x / b.x, a.y / b.y); }
static V2 operator*(V2 a, F32 b){ return vec2(a.x * b, a.y * b); }
static V2 operator*(F32 a, V2 b){ return vec2(a * b.x, a * b.y); }
#endif

static V2i vec2i(I32 x, I32 y){ V2i result = { x, y }; return result; }
static V2i v2i_scalar_mul(V2i a, I32 scalar){ return vec2i(a.x * scalar, a.y * scalar); }
static V2i v2i_add(V2i a, V2i b){ return vec2i(a.x + b.x, a.y + b.y); }
static V2i v2i_sub(V2i a, V2i b){ return vec2i(a.x - b.x, a.y - b.y); }
static I32 v2i_dot(V2i a, V2i b){ return a.x * b.x + a.y * b.y; }
static V2i v2i_negate(V2i a){ return vec2i(-a.x, -a.y); }

static V2 v2_from_v2i(V2i v){ return vec2((F32)v.x, (F32)v.y); }
static V2i v2i_from_v2(V2 v){ return vec2i((I32)v.x, (I32)v.y); }

//~ V3

static V3 vec3(F32 x, F32 y, F32 z){ V3 result = { x, y, z }; return result; }
static V3 vec3_set1(F32 x){ return vec3(x, x, x); }

static V3 v3_scalar_mul(V3 a, F32 scalar){ return vec3(a.x * scalar, a.y * scalar, a.z * scalar); }
static V3 v3_add(V3 a, V3 b){ return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
static V3 v3_sub(V3 a, V3 b){ return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
static V3 v3_negate(V3 a){ return vec3(-a.x, -a.y, -a.z); }
static F32 v3_dot(V3 a, V3 b){ return a.x * b.x + a.y * b.y + a.z * b.z; }
// NOTE(hanna - 2020-09-18): We work in a right hand coordinate system.
static V3 v3_cross(V3 a, V3 b){ return vec3(a.y * b.z - a.z * b.y,
                                             a.z * b.x - a.x * b.z,
                                             a.x * b.y - a.y * b.x); }
static F32 v3_length_squared(V3 v){ return SQUARE(v.x) + SQUARE(v.y) + SQUARE(v.z); }
static F32 v3_distance_squared(V3 a, V3 b){ return SQUARE(a.x - b.x) + SQUARE(a.y - b.y) + SQUARE(a.z - b.z); }
static V3 v3_mix(V3 a, F32 factor, V3 b){
  return v3_add( v3_scalar_mul(a, 1 - factor), v3_scalar_mul(b, factor) );
}
static void v3_swap(V3 *a, V3 *b){
  V3 tmp = *a;
  *a = *b;
  *b = tmp;
}

static V3 v3_normalize(V3 v){
  F32 length = sqrtf(v3_length_squared(v));
  return v3_scalar_mul(v, 1.f / length);
}

static I32 v3_min_abs_axis(V3 v){
  I32 result = 0;
  if(f32_absolute(v.e[1]) < f32_absolute(v.e[0])) result = 1;
  if(f32_absolute(v.e[2]) < f32_absolute(v.e[result])) result = 2;
  return result;
}
static I32 v3_min_axis(V3 v){
  I32 result = 0;
  if(v.e[1] < v.e[0]) result = 1;
  if(v.e[2] < v.e[result]) result = 2;
  return result;
}
static I32 v3_max_axis(V3 v){
  I32 result = 0;
  if(v.e[1] > v.e[0]) result = 1;
  if(v.e[2] > v.e[result]) result = 2;
  return result;
}

static F32 v3_min_element(V3 v){ return MINIMUM3(v.x, v.y, v.z); }
static F32 v3_max_element(V3 v){ return MAXIMUM3(v.x, v.y, v.z); }

static V3 v3_min(V3 a, V3 b){ return vec3(f32_min2(a.x, b.x), f32_min2(a.y, b.y), f32_min2(a.z, b.z)); }
static V3 v3_max(V3 a, V3 b){ return vec3(f32_max2(a.x, b.x), f32_max2(a.y, b.y), f32_max2(a.z, b.z)); }

static V3 v3_hadamard(V3 a, V3 b){ return vec3(a.x * b.x, a.y * b.y, a.z * b.z); }

#ifdef __cplusplus
static V3 operator+(V3 a, V3 b){ return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
static V3 operator+=(V3 &a, V3 b){ return a = a + b; }
static V3 operator-(V3 a, V3 b){ return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
static V3 operator-=(V3 &a, V3 b){ return a = a - b; }
static V3 operator*(V3 a, V3 b){ return vec3(a.x * b.x, a.y * b.y, a.z * b.z); } // Hadamard
static V3 operator/(V3 a, V3 b){ return vec3(a.x / b.x, a.y / b.y, a.z / b.z); }
static V3 operator*(V3 a, F32 b){ return vec3(a.x * b, a.y * b, a.z * b); }
static V3 operator*(F32 a, V3 b){ return vec3(a * b.x, a * b.y, a * b.z); }
#endif

//~ V3i

static V3i vec3i(I32 x, I32 y, I32 z){ V3i result = { x, y, z }; return result; }
static V3i v3i_scalar_mul(V3i a, I32 scalar){ return vec3i(a.x * scalar, a.y * scalar, a.z * scalar); }
static V3i v3i_add(V3i a, V3i b){ return vec3i(a.x + b.x, a.y + b.y, a.z + b.z); }
static V3i v3i_sub(V3i a, V3i b){ return vec3i(a.x - b.x, a.y - b.y, a.z - b.z); }
static I32 v3i_dot(V3i a, V3i b){ return a.x * b.x + a.y * b.y + a.z * b.z; }
// NOTE(hanna - 2020-09-18): We work in a right hand coordinate system.
static V3i v3i_cross(V3i a, V3i b){ return vec3i(a.y * b.z - a.z * b.y,
                                                  a.z * b.x - a.x * b.z,
                                                  a.x * b.y - a.y * b.x); }

static V3i v3i_from_v3(V3 v){ return vec3i((I32)v.x, (I32)v.y, (I32)v.z); }

//~ V4

static V4 v4(F32 x, F32 y, F32 z, F32 w){ V4 result = { x, y, z, w }; return result; }
static V4 vec4(F32 x, F32 y, F32 z, F32 w){ V4 result = { x, y, z, w }; return result; }
static V4 vec4_set1(F32 x){ return vec4(x, x, x, x); }
static V4 v4_scalar_mul(V4 a, F32 scalar){ return vec4(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar); }
static V4 v4_add(V4 a, V4 b){ return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
static V4 v4_sub(V4 a, V4 b){ return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
static F32 v4_dot(V4 a, V4 b){ return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
static V4 v4_mix(V4 a, F32 t, V4 b){ return v4_add( v4_scalar_mul(a, 1 - t), v4_scalar_mul(b, t) ); }

static bool v4_bitwise_equal(V4 a, V4 b){ return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }

static V3 v4_xyz(V4 v){ return vec3(v.x, v.y, v.z); }

//~ Mat2x2

static Mat2x2 mat2x2_transpose(Mat2x2 value){
  Mat2x2 result = {0};
  fjz(2) fiz(2){
    result.e[i][j] = value.e[j][i];
  }
  return result;
}
static Mat2x2 mat2x2_mul(Mat2x2 a, Mat2x2 b){
  Mat2x2 result = {0};
  fjz(2) fiz(2){
    result.e[j][i] = a.e[j][0] * b.e[0][i]
                   + a.e[j][1] * b.e[1][i];
  }
  return result;
}
static V2 mul_mat2x2_v2(Mat2x2 m, V2 v){
  V2 result = {0};
  result.x = v.x * m.e[0][0] + v.y * m.e[0][1];
  result.y = v.x * m.e[1][0] + v.y * m.e[1][1];
  return result;
}

static Mat2x2 mat2x2_rotation(F32 theta){
  Mat2x2 result = {0};
  F32 cos = cosf(theta);
  F32 sin = sinf(theta);
  result.e[0][0] = cos;
  result.e[0][1] = -sin;
  result.e[1][0] = sin;
  result.e[1][1] = cos;
  return result;
}

#ifdef __cplusplus
static Mat2x2 operator*(Mat2x2 a, Mat2x2 b){ return mat2x2_mul(a, b); }
static V2 operator*(Mat2x2 a, V2 b){ return mul_mat2x2_v2(a, b); }
#endif

//~ Mat4x4

static Mat4x4 mat4x4_transpose(Mat4x4 value){
  Mat4x4 result = {0};
  for(int j = 0; j < 4; ++j) for(int i = 0; i < 4; ++i){
    result.e[i][j] = value.e[j][i];
  }
  return result;
}
static Mat4x4 mat4x4_mul(Mat4x4 a, Mat4x4 b){
  Mat4x4 result = {0};
  fjz(4)fiz(4){
    result.e[j][i] = a.e[j][0] * b.e[0][i]
                   + a.e[j][1] * b.e[1][i]
                   + a.e[j][2] * b.e[2][i]
                   + a.e[j][3] * b.e[3][i];
  }
  return result;
}
static V4 mat4x4_transform_v4(Mat4x4 m, V4 v){
  V4 result = {0};
  result.x = v.x * m.e[0][0] + v.y * m.e[0][1] + v.z * m.e[0][2] + v.w * m.e[0][3];
  result.y = v.x * m.e[1][0] + v.y * m.e[1][1] + v.z * m.e[1][2] + v.w * m.e[1][3];
  result.z = v.x * m.e[2][0] + v.y * m.e[2][1] + v.z * m.e[2][2] + v.w * m.e[2][3];
  result.w = v.x * m.e[3][0] + v.y * m.e[3][1] + v.z * m.e[3][2] + v.w * m.e[3][3];
  return result;
}
static V3 mat4x4_transform_point(Mat4x4 m, V3 p){
  return v4_xyz(mat4x4_transform_v4(m, vec4(p.x, p.y, p.z, 1)));
}
static V3 mat4x4_transform_vector(Mat4x4 m, V3 p){
  return v4_xyz(mat4x4_transform_v4(m, vec4(p.x, p.y, p.z, 0)));
}

// Invert a transformation consisting only of a translation (first three elements of last column) followed by a rotation (3x3 matrix part)
static Mat4x4 mat4x4_translate_rotation_inverse(Mat4x4 m){
  Mat4x4 result = {0};

  // NOTE(hanna): Inverting a 3x3 rotation matrix is just a transpose
  fjz(3){
    fiz(3){
      result.e[j][i] = m.e[i][j];
    }
  }

  // NOTE(hanna): Inverting a translation is just negating the translation vector
  fiz(3){
    result.e[i][3] = -m.e[i][3];
  }

  result.e[3][3] = 1;

  return result;
}

#ifdef __cplusplus
static Mat4x4 operator*(Mat4x4 a, Mat4x4 b){ return mat4x4_mul(a, b); }
static V4 operator*(Mat4x4 m, V4 v){ return mat4x4_transform_v4(m, v); }
#endif

static V4 mat4x4_row(Mat4x4 mat, I32 index){
  return vec4(mat.e[index][0], mat.e[index][1], mat.e[index][2], mat.e[index][3]);
}
static V4 mat4x4_column(Mat4x4 mat, I32 index){
  return vec4(mat.e[0][index], mat.e[1][index], mat.e[2][index], mat.e[3][index]);
}

static Mat4x4 mat4x4_translate(V3 translate){
  Mat4x4 result = mat4x4_identity();
  result.e[0][3] = translate.x;
  result.e[1][3] = translate.y;
  result.e[2][3] = translate.z;
  return result;
}
static V3 mat4x4_extract_translate(Mat4x4 m){
  return vec3(m.e[0][3], m.e[1][3], m.e[2][3]);
}

static Mat4x4 mat4x4_scale(V3 scale){
  Mat4x4 result = {0};
  result.e[0][0] = scale.x;
  result.e[1][1] = scale.y;
  result.e[2][2] = scale.z;
  return result;
}

// NOTE(hanna): The following routines are all shamelessly stolen from Handmade Math
// https://github.com/HandmadeMath/HandmadeMath
// It is also licensed as public domain.

static Mat4x4 mat4x4_rotation_from_axis(F32 angle, V3 axis){
  Mat4x4 result = mat4x4_identity();

  axis = v3_normalize(axis);

  // TODO: Roll my own math routines!
  F32 sin_theta = sinf(angle);
  F32 cos_theta = cosf(angle);
  F32 cos_value = 1.0f - cos_theta;

  result.e[0][0] = (axis.x * axis.x * cos_value) + cos_theta;
  result.e[1][0] = (axis.x * axis.y * cos_value) + (axis.z * sin_theta);
  result.e[2][0] = (axis.x * axis.z * cos_value) - (axis.y * sin_theta);

  result.e[0][1] = (axis.y * axis.x * cos_value) - (axis.z * sin_theta);
  result.e[1][1] = (axis.y * axis.y * cos_value) + cos_theta;
  result.e[2][1] = (axis.y * axis.z * cos_value) + (axis.x * sin_theta);

  result.e[0][2] = (axis.z * axis.x * cos_value) + (axis.y * sin_theta);
  result.e[1][2] = (axis.z * axis.y * cos_value) - (axis.x * sin_theta);
  result.e[2][2] = (axis.z * axis.z * cos_value) + cos_theta;

  return result;
}

// fov specifies horizontal fov (note that Win32 #defines near and far)
static Mat4x4 mat4x4_perspective(F32 fov, F32 aspect_ratio, F32 n, F32 f){
  Mat4x4 result = {0};

  // TODO: Roll our own math routines!
  F32 cotangent = 1.0f / tanf(fov / 2.0f);
  result.e[0][0] = cotangent / aspect_ratio;
  result.e[1][1] = cotangent;
  result.e[3][2] = -1.0f;

  result.e[2][2] = -(n + f) / (f - n);
  result.e[2][3] = -(2.0f * n * f) / (f - n);

  return result;
}

//~ Mat3x3

static Mat3x3 mat3x3_mul(Mat3x3 a, Mat3x3 b){
  Mat3x3 result = {0};
  fjz(3)fiz(3){
    result.e[j][i] = a.e[j][0] * b.e[0][i]
                   + a.e[j][1] * b.e[1][i]
                   + a.e[j][2] * b.e[2][i];
  }
  return result;
}
static Mat3x3 mat3x3_transpose(Mat3x3 m){
  Mat3x3 result;
  fjz(3)fiz(3){
    result.e[j][i] = m.e[i][j];
  }
  return result;
}
static V3 mat3x3_transform_v3(Mat3x3 m, V3 v){
  V3 result;
  result.x = v.x * m.e[0][0] + v.y * m.e[0][1] + v.z * m.e[0][2];
  result.y = v.x * m.e[1][0] + v.y * m.e[1][1] + v.z * m.e[1][2];
  result.z = v.x * m.e[2][0] + v.y * m.e[2][1] + v.z * m.e[2][2];
  return result;
}

#ifdef __cplusplus
static Mat3x3 operator*(Mat3x3 a, Mat3x3 b){ return mat3x3_mul(a, b); }
static V3 operator*(Mat3x3 a, V3 b){ return mat3x3_transform_v3(a, b); }
#endif


//~ Affine2x3

typedef struct Affine2x3 Affine2x3;
struct Affine2x3{
  F32 e[2][3];
  /*
  NOTE(hanna): This represents a transformation corresponding to a 2x2 scale/rotate matrix combined with a translation.
  The 2x2 matrix is the elements

      e[0][0]   e[0][1]
      e[1][0]   e[1][1]

  and the translation corresponds to the elements

     e[0][2]
     e[1][2]

  See routines below which use this for details.
  */
};
CT_ASSERT(sizeof(Affine2x3) == 24); // Beware that this is not the smallest of structs!
#define AFFINE2x3_IDENTITY ( (Affine2x3){ .e[0][0] = 1, .e[1][1] = 1 } )

static Mat4x4 affine2x3_to_mat4x4(Affine2x3 t){
  Mat4x4 result = MAT4x4_IDENTITY;
  result.e[0][0] = t.e[0][0];
  result.e[0][1] = t.e[0][1];
  result.e[1][0] = t.e[1][0];
  result.e[1][1] = t.e[1][1];
  result.e[0][3] = t.e[0][2];
  result.e[1][3] = t.e[1][2];
  return result;
}

static V2 affine2x3_transform_v2(Affine2x3 t, V2 v){
  V2 result;
  result.x = v.x * t.e[0][0] + v.y * t.e[0][1] + t.e[0][2];
  result.y = v.x * t.e[1][0] + v.y * t.e[1][1] + t.e[1][2];
  return result;
}

static V2 affine2x3_inverse_transform_v2(Affine2x3 t, V2 v){
  F32 inv_det = 1.f / (t.e[0][0] * t.e[1][1] - t.e[0][1] * t.e[1][0]);

  v.x = inv_det * (v.x - t.e[0][2]);
  v.y = inv_det * (v.y - t.e[1][2]);

  V2 result;
  result.x = +v.x * t.e[1][1] - v.y * t.e[0][1];
  result.y = -v.x * t.e[1][0] + v.y * t.e[0][0];
  return result;
}

static F32 affine2x3_scale_x_squared(Affine2x3 t){ return SQUARE(t.e[0][0]) + SQUARE(t.e[0][1]); }
static F32 affine2x3_scale_y_squared(Affine2x3 t){ return SQUARE(t.e[1][0]) + SQUARE(t.e[1][1]); }

//~ Rect2

typedef struct Rect2 Rect2;
struct Rect2{
  union{
    struct{ V2 min, max; };
    struct{ F32 min_x, min_y, max_x, max_y; };
  };
};

static Rect2 rect2_f32(F32 min_x, F32 min_y, F32 max_x, F32 max_y){
  Rect2 result = {0};
  result.min_x = min_x;
  result.min_y = min_y;
  result.max_x = max_x;
  result.max_y = max_y;
  return result;
}
static Rect2 rect2(V2 min, V2 max){
  Rect2 result = {0};
  result.min = min;
  result.max = max;
  return result;
}

static Rect2 rect2_from_center_and_extents(V2 center, V2 extents){
  Rect2 result = {0};
  result.min = v2_sub(center, extents);
  result.max = v2_add(center, extents);
  return result;
}
static Rect2 rect2_hull_of_2_points(V2 P0, V2 P1){
  Rect2 result = {0};
  result.min_x = MINIMUM(P0.x, P1.x);
  result.min_y = MINIMUM(P0.y, P1.y);
  result.max_x = MAXIMUM(P0.x, P1.x);
  result.max_y = MAXIMUM(P0.y, P1.y);
  return result;
}
static Rect2 rect2_hull_of_3_points(V2 P0, V2 P1, V2 P2){
  Rect2 result = {0};
  result.min_x = MINIMUM3(P0.x, P1.x, P2.x);
  result.min_y = MINIMUM3(P0.y, P1.y, P2.y);
  result.max_x = MAXIMUM3(P0.x, P1.x, P2.x);
  result.max_y = MAXIMUM3(P0.y, P1.y, P2.y);
  return result;
}

static V2 rect2_center(Rect2 rect){
  return v2_scalar_mul(v2_add(rect.max, rect.min), 0.5f);
}

static V2 map_normalized_onto_rect2(V2 P, Rect2 rect){
  V2 result = {0};
  result.x = (1 - P.x) * rect.min_x + P.x * rect.max_x;
  result.y = (1 - P.y) * rect.min_y + P.y * rect.max_y;
  return result;
}

static bool rect2_intersects(Rect2 a, Rect2 b){
  bool x = a.max_x >= b.min_x && b.max_x >= a.min_x;
  bool y = a.max_y >= b.min_y && b.max_y >= a.min_y;
  return x && y;
}
static Rect2 rect2_intersection(Rect2 a, Rect2 b){
  Rect2 result = {0};
  result.min_x = MAXIMUM(a.min_x, b.min_x);
  result.min_y = MAXIMUM(a.min_y, b.min_y);
  result.max_x = MINIMUM(a.max_x, b.max_x);
  result.max_y = MINIMUM(a.max_y, b.max_y);
  return result;
}
static bool is_v2_in_rect2(V2 P, Rect2 rect){
  return rect.min_x <= P.x && P.x <= rect.max_x
      && rect.min_y <= P.y && P.y <= rect.max_y;
}

static Rect2 rect2_translate(Rect2 rect, V2 v){
  return rect2(v2_add(rect.min, v), v2_add(rect.max, v));
}

static Rect2 rect2_pad(Rect2 r, F32 padding){
  Rect2 result = r;
  result.min_x -= padding;
  result.min_y -= padding;
  result.max_x += padding;
  result.max_y += padding;
  return result;
}

static F32 rect2_dim_x(Rect2 rect){ return rect.max_x - rect.min_x; }
static F32 rect2_dim_y(Rect2 rect){ return rect.max_y - rect.min_y; }
static V2 rect2_dim(Rect2 rect){ return vec2(rect.max_x - rect.min_x, rect.max_y - rect.min_y); }

static Rect2 rect2_cut_left(Rect2 *r, F32 d){
  Rect2 result = *r;
  result.max_x = r->min_x + d;
  r->min_x += d;
  return result;
}
static Rect2 rect2_cut_right(Rect2 *r, F32 d){
  Rect2 result = *r;
  result.min_x = r->max_x - d;
  r->max_x -= d;
  return result;
}
static Rect2 rect2_cut_bottom(Rect2 *r, F32 d){
  Rect2 result = *r;
  result.max_y = r->min_y + d;
  r->min_y += d;
  return result;
}
static Rect2 rect2_cut_top(Rect2 *r, F32 d){
  Rect2 result = *r;
  result.min_y = r->max_y - d;
  r->max_y -= d;
  return result;
}

static Rect2 rect2_extend_left(Rect2 r, F32 d){
  Rect2 result = r;
  result.min_x -= d;
  return result;
}
static Rect2 rect2_extend_right(Rect2 r, F32 d){
  Rect2 result = r;
  result.max_x += d;
  return result;
}
static Rect2 rect2_extend_bottom(Rect2 r, F32 d){
  Rect2 result = r;
  result.min_y -= d;
  return result;
}
static Rect2 rect2_extend_top(Rect2 r, F32 d){
  Rect2 result = r;
  result.max_y += d;
  return result;
}

static Rect2 rect2_cut_margins(Rect2 r, F32 d){
  Rect2 result = r;
  result.min_x += d;
  result.max_x -= d;
  result.min_y += d;
  result.max_y -= d;
  return result;
}
static Rect2 rect2_cut_margins_xy(Rect2 r, V2 d){
  Rect2 result = r;
  result.min_x += d.x;
  result.max_x -= d.x;
  result.min_y += d.y;
  result.max_y -= d.y;
  return result;
}

// NOTE: This is for user interface purposes, so we prefer min_x and max_y being correct.
static Rect2 rect2_fit_other_rect_inside(Rect2 inner, Rect2 outer){
  Rect2 result = inner;
  if(inner.min_x < outer.min_x){
    F32 d = outer.min_x - inner.min_x;
    result.min_x += d;
    result.max_x += d;
  }else if(inner.max_x > outer.max_x){
    F32 d = outer.max_x - inner.max_x;
    result.min_x += d;
    result.max_x += d;
  }

  if(inner.max_y > outer.max_y){
    F32 d = outer.max_y - inner.max_y;
    result.min_y += d;
    result.max_y += d;
  }else if(inner.min_y < outer.min_y){
    F32 d = outer.min_y - inner.min_y;
    result.min_y += d;
    result.max_y += d;
  }
  return result;
}

static F32 rect2_v2_distance_sq(Rect2 r, V2 p){
  F32 result;
  bool in_x = IN_RANGE(r.min_x <=, p.x, <= r.max_x);
  bool in_y = IN_RANGE(r.min_y <=, p.y, <= r.max_y);
  if(in_x && in_y){
    result = 0;
  }else if(in_x){
    F32 d0 = SQUARE(p.y - r.max_y);
    F32 d1 = SQUARE(p.y - r.min_y);
    result = MINIMUM(d0, d1);
  }else if(in_y){
    F32 d0 = SQUARE(p.x - r.max_x);
    F32 d1 = SQUARE(p.x - r.min_x);
    result = MINIMUM(d0, d1);
  }else{
    F32 d0 = v2_distance_sq(p, vec2(r.min_x, r.min_y));
    F32 d1 = v2_distance_sq(p, vec2(r.max_x, r.min_y));
    F32 d2 = v2_distance_sq(p, vec2(r.min_x, r.max_y));
    F32 d3 = v2_distance_sq(p, vec2(r.max_x, r.max_y));
    result = f32_min4(d0, d1, d2, d3);
  }
  return result;
}

static bool rect2_overlaps(Rect2 a, Rect2 b){
  bool x = (b.min_x < a.max_x && a.min_x < b.max_x);
  bool y = (b.min_y < a.max_y && a.min_y < b.max_y);
  return x && y;
}

typedef struct Rect2i Rect2i;
struct Rect2i{
  union{
    struct{ I32 min_x, min_y, max_x, max_y; };
  };
};

//~ Rect3

typedef struct Rect3 Rect3;
struct Rect3{
  V3 min, max;
};
static Rect3 rect3_from_center_and_extents(V3 center, V3 extents){
  Rect3 result = {0};
  result.min = v3_sub(center, extents);
  result.max = v3_add(center, extents);
  return result;
}

static Rect3 rect3_from_points(V3 *ps, U32 count){
  Rect3 result = { vec3_set1(F32_INFINITY), vec3_set1(-F32_INFINITY) };
  fiz(count){
    result.min = v3_min(result.min, ps[i]);
    result.max = v3_max(result.max, ps[i]);
  }
  return result;
}
static Rect3 rect3_from_rect3s(Rect3 a, Rect3 b){
  Rect3 result = {0};
  result.min = v3_min(a.min, b.min);
  result.max = v3_max(a.max, b.max);
  return result;
}
static Rect3 rect3_extend_with_point(Rect3 a, V3 p){
  Rect3 result = {0};
  result.min = v3_min(a.min, p);
  result.max = v3_max(a.max, p);
  return result;
}

static V3 rect3_center(Rect3 rect){
  return vec3(0.5f * (rect.min.x + rect.max.x), 0.5f * (rect.min.y + rect.max.y), 0.5f * (rect.min.z + rect.max.z));
}
static bool rect3_intersects(Rect3 a, Rect3 b){
  bool x = a.max.x >= b.min.x && b.max.x >= a.min.x;
  bool y = a.max.y >= b.min.y && b.max.y >= a.min.y;
  bool z = a.max.z >= b.min.z && b.max.z >= a.min.z;
  return x && y && z;
}
static V3 rect3_dim(Rect3 rect){
  V3 result = v3_sub(rect.max, rect.min);
  return result;
}

// Returns `true` if `outer` fully contains `inner`
static bool rect3_contains(Rect3 outer, Rect3 inner){
  bool result = false;
  if(1
    && outer.min.e[0] <= inner.min.e[0] && inner.max.e[0] <= outer.max.e[0]
    && outer.min.e[1] <= inner.min.e[1] && inner.max.e[1] <= outer.max.e[1]
    && outer.min.e[2] <= inner.min.e[2] && inner.max.e[2] <= outer.max.e[2])
  {
    result = true;
  }
  return result;
}

//
// BEGIN PCG RANDOM
//
// This is a modified version of O'Neill's code which has the following license:
//
// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
//

typedef struct { U64 state;  U64 inc; } PCG_State;

static PCG_State pcg_create_with_os_entropy(){
  PCG_State result = {0};
  result.state = os_get_entropy_u64();
  result.inc = os_get_entropy_u64() | LIT_U64(1);
  return result;
}

static U32 pcg_random_u32(PCG_State* rng){
  assert(rng->inc && "Looks like you didn't initialize your random generator!");
  U64 oldstate = rng->state;
  // Advance internal state
  rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
  // Calculate output function (XSH RR), uses old state for max ILP
  U32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
  U32 rot = oldstate >> 59u;
  return (xorshifted >> rot) | (xorshifted << ((0-rot) & 31));
}

static U32 pcg_random_u32_range(PCG_State *rng, U32 min, U32 max){
  // TODO: Here we should probably verify correctness sometime in the future
  assert(max >= min);
  U32 diff = max - min + 1;
  U32 limit = UINT32_MAX - (UINT32_MAX % diff);
  U32 result;
  do{
    result = pcg_random_u32(rng) % diff;
  }while(result > limit);
  result += min;
  return result;
}

static U64 pcg_random_u64(PCG_State *rng){
  // TODO: This is apparently not a good way of doing this, but I am too lazy too fix it for now.
  return (U64)pcg_random_u32(rng) | ((U64)pcg_random_u32(rng) << LIT_U64(32));
}
static U64 pcg_random_u64_nonzero(PCG_State *rng){
  U64 result = pcg_random_u64(rng);
  if(result == 0){
    result = 1;
  }
  return result;
}

// NOTE(hanna): With my limited understanding of random number generation this should
// be a okay way of generating a random F32
static F32 pcg_random_f32_01(PCG_State *rng){
  U32 value = pcg_random_u32(rng);
  return (F32)((F64)value / (F64)0xffffffff);
}
static F32 pcg_random_f32(PCG_State *rng, F32 a, F32 b){
  return pcg_random_f32_01(rng) * (b - a) + a;
}
static V2 pcg_random_v2_in_rect(PCG_State *rng, F32 min_x, F32 min_y, F32 max_x, F32 max_y){
  V2 result = {0};
  result.x = pcg_random_f32(rng, min_x, max_x);
  result.y = pcg_random_f32(rng, min_y, max_y);
  return result;
}

// TODO: We should really revise these routines:
static V2 pcg_random_v2_in_unit_circle(PCG_State *rng){
  V2 result;
  // TODO: We might want to look into doing this not with a loop but with some clever method, e.g. generating normally distributed coordinates.
  do{
    result = pcg_random_v2_in_rect(rng, -1, -1, 1, 1);
  }while(v2_length_sq(result) > SQUARE(1));
  return result;
}
static V2 pcg_random_v2_on_unit_circle(PCG_State *rng){
  V2 result = pcg_random_v2_in_unit_circle(rng);
  result = v2_normalize(result);
  return result;
}

//
// END PCG RANDOM
//

// NOTE(hanna - 2021-05-14): I don't know much about PRNGs but here are some resources I have found:
// https://espadrine.github.io/blog/posts/a-primer-on-randomness.html
// https://burtleburtle.net/bob/rand/talksmall.html
//

//~ Dynamic arrays

typedef struct Array_any Array_any;
struct Array_any{
  void *e;
  U32 count;
  U32 capacity;
};
CT_ASSERT(sizeof(Array_any) == 16);

// TODO IMPORTANT: We should verify that the trick with Array_any does not break C aliasing rules

#define Array(_Type_) COMBINE2(Array_, _Type_)
#define DECLARE_ARRAY_TYPE(_Type_) \
  typedef union Array(_Type_){ \
    struct{ _Type_ *e; U32 count; U32 capacity; }; \
    Array_any as_any_array; \
  } Array(_Type_); \
  static Array(_Type_) COMBINE2(Array(_Type_), _create)(Allocator *allocator){ \
    Array(_Type_) result = {0}; \
    result.e = (_Type_*)allocator_get_stub(allocator); \
    return result; \
  }

DECLARE_ARRAY_TYPE(bool);

DECLARE_ARRAY_TYPE(U8);
DECLARE_ARRAY_TYPE(U16);
DECLARE_ARRAY_TYPE(U32);
DECLARE_ARRAY_TYPE(U64);
DECLARE_ARRAY_TYPE(I8);
DECLARE_ARRAY_TYPE(I16);
DECLARE_ARRAY_TYPE(I32);
DECLARE_ARRAY_TYPE(I64);
DECLARE_ARRAY_TYPE(F32);
DECLARE_ARRAY_TYPE(F64);
DECLARE_ARRAY_TYPE(String);
DECLARE_ARRAY_TYPE(V2);
DECLARE_ARRAY_TYPE(V3);
DECLARE_ARRAY_TYPE(V4);

DECLARE_ARRAY_TYPE(Array(U8));
DECLARE_ARRAY_TYPE(Array(U16));
DECLARE_ARRAY_TYPE(Array(U32));
DECLARE_ARRAY_TYPE(Array(U64));
DECLARE_ARRAY_TYPE(Array(I8));
DECLARE_ARRAY_TYPE(Array(I16));
DECLARE_ARRAY_TYPE(Array(I32));
DECLARE_ARRAY_TYPE(Array(I64));
DECLARE_ARRAY_TYPE(Array(F32));
DECLARE_ARRAY_TYPE(Array(F64));
DECLARE_ARRAY_TYPE(Array(String));
DECLARE_ARRAY_TYPE(Array(V2));
DECLARE_ARRAY_TYPE(Array(V3));
DECLARE_ARRAY_TYPE(Array(V4));

typedef void *RawPtr;
DECLARE_ARRAY_TYPE(RawPtr);

#define array_create(_Type_, _allocator_) ( COMBINE2(Array(_Type_), _create)((_allocator_)) )

#define array_reset(_array_) ( (_array_)->count = 0 )

static void _array_reserve(Array_any *array, U64 needed_capacity, size_t element_size, size_t element_align){
  assert(needed_capacity < UINT32_MAX);
  assert(element_size < UINT32_MAX);
  assert(array->e && "Before using a array you need to initialize it with array_create");

  if(array->capacity < needed_capacity){
    U64 old_size = array->capacity * (U64)element_size;
    size_t expand_size = needed_capacity * element_size;
    if(allocator_expand(array->e, old_size, expand_size)){
      array->capacity = needed_capacity;
    }else{
      U64 new_capacity = MAXIMUM(needed_capacity, array->capacity * 2);
      U64 realloc_size = new_capacity * (U64)element_size;
      allocator_realloc_noclear(&array->e, old_size, realloc_size, element_align);
      array->capacity = new_capacity;
      assert(array->e);
    }
  }
}
#define array_reserve(_array_, ...) \
  (_array_reserve(&(_array_)->as_any_array, (__VA_ARGS__), sizeof((_array_)->e[0]), ALIGNOF_EXPR((_array_)->e[0])))

static void _array_destroy(Array_any *array, size_t element_size){
  size_t size = array->capacity * element_size;
  allocator_free(array->e, size);
  clear_item(array);
}
#define array_destroy(_array_) (_array_destroy(&(_array_)->as_any_array, sizeof((_array_)->e[0])), clear_item(_array_))

#define array_allocator(_array_) ( get_allocator((_array_)->e) )

static void _array_set_count_clear(Array_any *array, U64 new_count, size_t element_size, size_t element_align){
  _array_reserve(array, new_count, element_size, element_align);
  if(new_count > array->count){
    memset((U8*)array->e + element_size * array->count,
           0,
           (new_count - array->count) * element_size);
  }
  array->count = new_count;
}
#define array_set_count_clear(_array_, ...) \
  ( _array_set_count_clear(&(_array_)->as_any_array, (__VA_ARGS__), sizeof((_array_)->e[0]), ALIGNOF_EXPR((_array_)->e[0])) )

static void _array_set_count_noclear(Array_any *array, U64 new_count, size_t element_size, size_t element_align){
  _array_reserve(array, new_count, element_size, element_align);
  array->count = new_count;
}
#define array_set_count_noclear(_array_, ...) \
  (_array_set_count_noclear(&(_array_)->as_any_array, (__VA_ARGS__), sizeof((_array_)->e[0]), ALIGNOF_EXPR((_array_)->e[0])))

static I64 _array_get_element(Array_any *array, I64 index){
  assert(index >= 0);
  assert(index < array->count);
  I64 result = index;
  return result;
}
#define array_get_element(_array_, ...) \
  ((_array_)->e + _array_get_element((__VA_ARGS__)))

#define array_push(_array_, ...) \
  (_array_reserve(&(_array_)->as_any_array, (U64)(_array_)->count + 1, sizeof((_array_)->e[0]), ALIGNOF_EXPR((_array_)->e[0])), \
   ((_array_)->e[(_array_)->count] = (__VA_ARGS__)), \
   ((_array_)->count += 1), \
   &((_array_)->e[(_array_)->count - 1]))

static void _array_insert(Array_any *array, U64 insert_index, size_t element_size, size_t element_align){
  assert(insert_index <= array->count);

  _array_reserve(array, array->count + 1, element_size, element_align);
  memmove((U8*)array->e + (insert_index + 1) * element_size,
          (U8*)array->e + (insert_index + 0) * element_size,
          element_size * (array->count - insert_index));
}
#define array_insert(_array_, _index_, ...) \
  (_array_insert(&(_array_)->as_any_array, (_index_), sizeof((_array_)->e[0]), ALIGNOF_EXPR((_array_)->e[0])), \
   ((_array_)->e[(_index_)] = (__VA_ARGS__)), \
   ((_array_)->count += 1), /* NOTE: This allows for doing things like array_insert(&array, array.count, ...) */ \
   &((_array_)->e[(_index_)]))

static void _array_delete_range(Array_any *array, size_t element_size, U64 begin, U64 end){
  assert(begin <= end);
  assert(end <= array->count);

  memmove((U8*)array->e + begin * element_size,
          (U8*)array->e + end * element_size,
          element_size * (array->count - end));
  array->count -= (end - begin);
}
#define array_delete_at_slow(_array_, _index_) \
  (_array_delete_range(&(_array_)->as_any_array, sizeof((_array_)->e[0]), (_index_), (_index_) + 1))

#define array_delete_range(_array_, _begin_, _end_) \
  (_array_delete_range(&(_array_)->as_any_array, sizeof((_array_)->e[0]), (_begin_), (_end_)))

#define array_delete_at_fast(_array_, _index_) \
  (assert((_array_)->count > 0), \
   (_array_)->e[(_index_)] = (_array_)->e[(_array_)->count - 1], \
   (_array_)->count -= 1)

#define array_pop(_array_) \
  (assert((_array_)->count > 0), (_array_)->e[(_array_)->count -= 1])

static Array_any _array_copy(Array_any *src, size_t element_size, size_t element_align){
  Array_any result = {0};
  Allocator *allocator = get_allocator(src->e);
  result.e = allocator_get_stub(allocator);
  _array_reserve(&result, src->count, element_size, element_align);
  memcpy(result.e, src->e, (size_t)element_size * (size_t)src->count);
  return result;
}
#define array_copy(_dst_, _src_) \
  ( (_dst_)->as_any_array = _array_copy(&(_src_)->as_any_array, sizeof((_src_)->e[0]), ALIGNOF_EXPR((_src_)->e[0])) )



#if 0 // Ended up not being used, so commented out for now as it was never inspected in a debugger.
static void _array_concatenate(Array_any *src, void *data, size_t element_count, size_t element_size, size_t element_align){
  assert(element_count < UINT32_MAX);
  assert(element_size < UINT32_MAX);

  size_t old_count = src->count;
  _array_set_count_noclear(src, src->count + element_count, element_size, element_align);
  memcpy((U8*)src->e + old_count * element_size,
         data,
         element_count * element_size);
}
#define array_concatenate(_dst_, _data_, _count_) \
  ( assert(sizeof((_dst_)->e[0]) == sizeof((_data_)[0]) && "Obvious type mismatch going on here!"), \
    _array_concatenate((_dst_), (_data_), (_count_), sizeof((_dst_)->e[0]), ALIGNOF_EXPR((_dst_)->e[0])) )
#endif

//
// Array(U8) STUFF
//

static Array(U8) array_u8_create_from_data(Allocator *allocator, U8 *data, I64 count){
  Array(U8) result = array_create(U8, allocator);
  array_set_count_noclear(&result, count);
  memcpy(result.e, data, count);
  return result;
}
static Array(U8) array_u8_create_from_string(Allocator *allocator, String string){
  return array_u8_create_from_data(allocator, string.data, string.size);
}

static String array_u8_as_string(Array(U8) array){
  String result = {0};
  result.data = array.e;
  result.size = array.count;
  return result;
}
static void array_u8_write_data(Array(U8) *array, I64 offset, U8 *data, I64 size){
  assert(offset >= 0);
  assert(offset + size < UINT32_MAX);
  if(offset + size > array->count){
    // TODO: An array_reserve_with_zeros call would be appreciated here!
    array_reserve(array, offset + size);
    while(offset + size > array->count){
      array_push(array, 0);
    }
  }
  memcpy(array->e + offset, data, size);
}
static void array_u8_push_data(Array(U8) *array, U8 *data, I64 size){
  array_reserve(array, array->count + size);
  fiz(size){
    array_push(array, data[i]);
  }
}
static void array_u8_push_string(Array(U8) *array, String string){
  array_u8_push_data(array, string.data, string.size);
}

// Replaces the contents of the interval [begin, end) with `data` (an array with `count` number of elements).
static void array_u8_replace(Array(U8) *array, I64 begin, I64 end, U8 *data, I64 count){
  assert(0 <= begin);
  assert(begin <= end);
  assert(end <= array->count);

  U32 move_count = array->count - end;
  array_set_count_noclear(array, array->count - (end - begin) + count);

  memmove(array->e + begin + count,
          array->e + end,
          move_count);
  memcpy(array->e + begin,
         data,
         count);
}
static void array_u8_replace_with_string(Array(U8) *array, I64 begin, I64 end, String value){
  array_u8_replace(array, begin, end, value.data, value.size);
}

static void array_u8_vprintf(Array(U8) *array, const char *format, va_list list1){
  va_list list2;
  va_copy(list2, list1);

  int size = stbsp_vsnprintf(NULL, 0, format, list1);
  va_end(list1);

  array_reserve(array, array->count + size + 1);

  stbsp_vsnprintf((char*)array->e + array->count, size + 1, format, list2);
  array_set_count_noclear(array, array->count + size);

  va_end(list2);
}
static void array_u8_printf(Array(U8) *array, const char *format, ...){
  va_list list;
  va_start(list, format);
  array_u8_vprintf(array, format, list);
  va_end(list);
}

//~ UTF32 Strings!

typedef struct StringUTF32 StringUTF32;
struct StringUTF32{
  U32 *data;
  size_t count;
};
CT_ASSERT(sizeof(StringUTF32) == 16);

static StringUTF32 string_utf32_from_array_u32(Array(U32) array){
  StringUTF32 result = {0};
  result.data = array.e;
  result.count = array.count;
  return result;
}
static bool string_utf32_equals(StringUTF32 a, StringUTF32 b){
  return memory_equals(a.data, a.count * 4, b.data, b.count * 4);
}
static StringUTF32 string_utf32_from_utf8(Allocator *allocator, String string){
  Array(U32) result = array_create(U32, allocator);
  array_reserve(&result, string.size); // Expect most input to be ASCII.

  for(UTF8Iterator iter = iterate_utf8(string);
    iter.valid;
    advance_utf8_iterator(&iter))
  {
    array_push(&result, iter.codepoint);
  }

  return string_utf32_from_array_u32(result);
}


static String string_utf8_from_codepoint(U8 buf[4], U32 codepoint){
  String result = {0};
  result.data = buf;
  result.size = encode_utf8(codepoint, buf, buf + 4);
  return result;
}
static void array_u8_push_codepoint(Array(U8) *array, U32 codepoint){
  U8 buf[4];
  String string = string_utf8_from_codepoint(buf, codepoint);
  array_u8_push_string(array, string);
}

static String string_utf8_from_utf32(Allocator *allocator, StringUTF32 utf32){
  Array(U8) result = array_create(U8, allocator);

  fiz(utf32.count){
    U8 buffer[4];
    U32 bytes = encode_utf8(utf32.data[i], buffer, buffer + sizeof(buffer));
    if(bytes == 0){
      clear_item(&result);
      break;
    }
    array_u8_push_data(&result, buffer, bytes);
  }

  return array_u8_as_string(result);
}


//~ UTF16 Strings!

typedef struct StringUTF16 StringUTF16;
struct StringUTF16{
  U16 *data;
  size_t count;
};
static StringUTF16 string_utf16_create(U16 *data, U32 count){
  StringUTF16 result = {0};
  result.data = data;
  result.count = count;
  return result;
}

static StringUTF16 string_utf16_from_array_u16(Array(U16) array){
  StringUTF16 result = {0};
  result.data = array.e;
  result.count = array.count;
  return result;
}

static size_t utf16_encode_codepoint(U16 data[2], U32 codepoint){
  // NOTE(hanna - 2022-12-20): I Wikipediaed to write this code: https://en.wikipedia.org/wiki/UTF-16#Description
  size_t result = 0;
  if(IN_RANGE(0 <=,codepoint,<= 0xD7FF) || IN_RANGE(0xE000 <=, codepoint, <= 0xFFFF)){
    data[0] = (U16)codepoint;
    result = 1;
  }else if(IN_RANGE(0x10000 <=, codepoint, <= 0x10FFFF)){
    U32 u = codepoint - 0x10000;
    U16 high = u >> 10;
    U16 low = u & LIT_U32(0x3ff);

    data[0] = LIT_U32(0xD800) + high;
    data[1] = LIT_U32(0xDC00) + low;
    result = 2;
  }
  return result;
}

// NOTE(hanna): This doesn't do much in terms of error checking!!
static U32 utf16_decode_codepoint(U16 data[2], U32 *_codepoint){
  // NOTE(hanna - 2022-12-22): I Wikipediaed to write this code: https://en.wikipedia.org/wiki/UTF-16#Description
  U32 codepoint = INVALID_CODEPOINT;
  U32 result = 0;

  if(IN_RANGE(0x0000 <=, data[0], <= 0xD7FF) || IN_RANGE(0xE000 <=, data[0], <= 0xFFFF)){
    codepoint = data[0];
    result = 1;
  }else{
    codepoint = ((data[0] - LIT_U32(0xD800)) << 10) | (data[1] - 0xDC00);
    result = 2;
  }

  if(_codepoint) *_codepoint = codepoint;
  return result;
}

// This mostly just exists for interfacing with the Win32 API
static void string_utf8_to_utf16_array(Array(U16) *out, String string){
  for(UTF8Iterator iter = iterate_utf8(string);
    iter.valid;
    advance_utf8_iterator(&iter))
  {
    U16 data[2];
    U32 count = utf16_encode_codepoint(data, iter.codepoint);
    fiz(count){
      array_push(out, data[i]);
    }
  }
}

static StringUTF16 string_utf16_from_cstring(wchar_t *wide){
  StringUTF16 result = {0};
  result.data = (U16*)wide;
  while(wide[result.count]){
    result.count += 1;
  }
  return result;
}

static void string_utf16_to_utf8_array(Array(U8) *out, StringUTF16 string){
  for(I64 cursor = 0; cursor < string.count;){
    U16 data[2];
    data[0] = string.data[cursor + 0];
    if(cursor + 1 < string.count){
      data[1] = string.data[cursor + 1];
    }
    U32 codepoint;
    cursor += utf16_decode_codepoint(data, &codepoint);
    if(codepoint == INVALID_CODEPOINT){
      break; // TODO: Don't just silentlty fail, do something!
    }
    array_u8_push_codepoint(out, codepoint);
  }
}

//
// FILE UTILITY
//

typedef struct EntireFile EntireFile;
struct EntireFile{
  bool ok;
  U8 *data;
  I64 size;
};
static EntireFile read_entire_file(String path, Allocator *allocator){
  EntireFile result = {0};

  OSFile file = os_open_file_input(path);
  if(file.value){
    I64 size = os_get_file_size(file);
    U8 *data = allocator_push_items_noclear(allocator, U8, size);
    if(os_read_from_file(file, 0, data, size)){
      result.ok = true;
      result.data = data;
      result.size = size;
    }
  }
  os_close_file(file);

  return result;
}
static String entire_file_as_string(EntireFile entire_file){
  assert(entire_file.ok);
  return string_create(entire_file.data, entire_file.size);
}

static bool dump_string_to_file(String path, String string){
  bool result = false;

  OSFile file = os_open_file_output(path);
  if(file.value){
    bool error = false;
    os_write_to_file(file, 0, string.data, string.size, &error);
    result = !error;
  }
  os_close_file(file);

  return result;
}

static void string_to_lines(Array(String) *out, String text){
  I64 cursor = 0;
  while(cursor < text.size){
    I64 line_begin = cursor;
    while(cursor < text.size && text.data[cursor] != '\n'){
      cursor += 1;
    }
    I64 line_end = cursor;

    // Detect CRLF (TODO: Test this code!)
    if(1 < line_end && line_end < text.size && text.data[line_end - 1] == '\r'){
      line_end -= 1;
    }
    cursor += 1; // skip '\n'

    String line = substring(text, line_begin, line_end);
    array_push(out, line);
  }
}

static void string_split_ascii_no_runs(Array(String) *out, String string, char c){
  I64 cursor = 0;
  while(cursor < string.size){
    I64 begin = cursor;
    while(cursor < string.size && string.data[cursor] != c){
      cursor += 1;
    }
    I64 end = cursor;
    while(cursor < string.size && string.data[cursor] == c){
      cursor += 1;
    }

    array_push(out, substring(string, begin, end));
  }
}

static bool codepoint_is_horizontal_whitespace(U32 codepoint){
  return codepoint == ' ' || codepoint == '\t';
}
static void string_split_at_horizontal_whitespace_no_runs(Array(String) *out, String string){
  I64 cursor = 0;

  while(cursor < string.size && codepoint_is_horizontal_whitespace(string.data[cursor])){
    cursor += 1;
  }

  while(cursor < string.size){
    I64 begin = cursor;
    while(cursor < string.size && !codepoint_is_horizontal_whitespace(string.data[cursor])){
      cursor += 1;
    }
    I64 end = cursor;
    while(cursor < string.size && codepoint_is_horizontal_whitespace(string.data[cursor])){
      cursor += 1;
    }

    array_push(out, substring(string, begin, end));
  }
}

static I64 string_beginning_of_line(String source, I64 offset){
  I64 result = offset;
  while(result > 0 && source.data[result - 1] != '\n'){
    result -= 1;
  }
  return result;
}
static I64 string_end_of_line(String source, I64 offset){
  I64 result = offset;
  while(result < source.size && source.data[result] != '\n'){
    result += 1;
  }
  return result;
}

static bool string_prev_line(String source, I64 *_line_begin, I64 *_line_end){
  I64 line_begin = *_line_begin;
  I64 line_end = *_line_end;

  bool result = (line_begin > 0);
  if(result){
    line_end = line_begin - 1;
    line_begin = string_beginning_of_line(source, line_begin - 1);
  }

  *_line_begin = line_begin;
  *_line_end = line_end;
  return result;
}

static bool string_next_line(String source, I64 *_line_begin, I64 *_line_end){
  I64 line_begin = *_line_begin;
  I64 line_end = *_line_end;

  bool result = (line_end < source.size);
  if(result){
    line_begin = line_end + 1;
    line_end = string_end_of_line(source, line_end + 1);
  }

  *_line_begin = line_begin;
  *_line_end = line_end;
  return result;
}


//
// FILE PATHS UTILITY
//

// NOTE(hanna): This struct represents an immutable file path.
typedef struct FilePath FilePath;
struct FilePath{
  bool invalid;
  bool absolute;
  Array(String) components;
  String as_string;
};
DECLARE_ARRAY_TYPE(FilePath);

static char* file_path_as_cstring(FilePath path){
  return (char*)path.as_string.data;
}

static FilePath file_path_begin(Allocator *allocator){
  FilePath result = {0};
  result.components = array_create(String, allocator);
  return result;
}

static void file_path_end(FilePath *path, Allocator *allocator){
  Array(U8) string = array_create(U8, allocator);
  assert(!path->invalid);
#if OS_LINUX
  fiz(path->components.count){
    if(i != 0 || path->absolute) array_push(&string, '/');
    array_u8_push_string(&string, path->components.e[i]);
  }
#else
//#error "Unknown OS"
#endif
  array_push(&string, '\0');

  path->as_string = array_u8_as_string(string);
}

static FilePath file_path_from_string(String string, Allocator *allocator){
  FilePath result = file_path_begin(allocator);

#if OS_LINUX
  if(string.size){
    I64 cursor = 0;

    if(string.data[0] == '/'){
      result.absolute = true;
    }

    while(cursor < string.size){
      // Skip possibly duplicated slashes
      while(cursor < string.size && string.data[cursor] == '/'){
        cursor += 1;
      }

      I64 begin = cursor;
      while(cursor < string.size && string.data[cursor] != '/'){
        cursor += 1;
      }
      I64 end = cursor;

      if(end > begin){
        String component = substring(string, begin, end);

        if(string_equals(component, LIT_STR(".."))){
          if(result.components.count > 0){
            array_pop(&result.components);
          }else{
            result.invalid = true;
          }
        }else if(string_equals(component, LIT_STR("."))){
          // Do nothing.
        }else{
          array_push(&result.components, component);
        }
      }
    }
  }
#elif OS_WINDOWS

//#error "TODO: Implement"
#else
#error "Unknown OS"
#endif

  file_path_end(&result, allocator);

  return result;
}

static FilePath file_path_relative_from_components(String *components, I32 count, Allocator *allocator){
  FilePath result = file_path_begin(allocator);

  array_reserve(&result.components, count);

  fiz(count){
    array_push(&result.components, components[i]);
  }

  file_path_end(&result, allocator);
  return result;
}
static FilePath file_path_relative_from_components1(String a, Allocator *allocator){
  String components[1] = { a};
  return file_path_relative_from_components(components, 1, allocator);
}
static FilePath file_path_relative_from_components2(String a, String b, Allocator *allocator){
  String components[2] = { a, b };
  return file_path_relative_from_components(components, 2, allocator);
}
static FilePath file_path_relative_from_components3(String a, String b, String c, Allocator *allocator){
  String components[3] = { a, b, c };
  return file_path_relative_from_components(components, 3, allocator);
}

static FilePath file_path_join(FilePath *paths, I32 count, Allocator *allocator){
  FilePath result = file_path_begin(allocator);

  if(count > 0){
    if(paths[0].absolute){
      result.absolute = true;
    }

    fjz(count){
      FilePath path = paths[j];
      if(path.invalid){ result.invalid = true; }
      fiz(path.components.count){
        array_push(&result.components, path.components.e[i]);
      }
    }
  }

  file_path_end(&result, allocator);

  return result;
}
static FilePath file_path_join2(FilePath a, FilePath b, Allocator *allocator){
  assert(!a.invalid);
  assert(!b.invalid);

  FilePath paths[2] = { a, b };

  return file_path_join(paths, 2, allocator);
}

static FilePath file_path_join2_printf(FilePath a, Allocator *allocator, const char *format, ...){
  va_list list;
  va_start(list, format);
  FilePath b = file_path_from_string(allocator_push_vprintf(allocator, format, list), allocator);
  va_end(list);
  return file_path_join2(a, b, allocator);
}

static FilePath file_path_join3(FilePath a, FilePath b, FilePath c, Allocator *allocator){
  FilePath paths[3] = { a, b, c };

  return file_path_join(paths, 3, allocator);
}

static FilePath file_path_go_up(FilePath path, I32 count, Allocator *allocator){
  FilePath result = file_path_begin(allocator);

  if(path.invalid){ result.invalid = true; }

  if(path.absolute){ result.absolute = true; }

  if(path.components.count >= count){
    result.components = array_create(String, allocator);
    array_reserve(&result.components, path.components.count - count);
    fiz(path.components.count - count){
      array_push(&result.components, path.components.e[i]);
    }
  }else{
    result.invalid = true;
  }

  file_path_end(&result, allocator);

  return result;
}

static FilePath file_path_to_absolute(FilePath relative_to, FilePath path, Allocator *allocator){
  FilePath result;
  if(path.absolute){
    result = path;
  }else{
    result = file_path_join2(relative_to, path, allocator);
  }
  return result;
}

static FilePath file_path_copy(FilePath path, Allocator *allocator){
  FilePath result = file_path_begin(allocator);

  result.absolute = path.absolute;
  result.components = array_create(String, allocator);
  array_reserve(&result.components, path.components.count);

  fiz(path.components.count){
    array_push(&result.components, allocator_push_string(allocator, path.components.e[i]));
  }

  file_path_end(&result, allocator);

  return result;
}

//
// FLOAT UTILITY
//

typedef union IEEE754_F64 IEEE754_F64;
union IEEE754_F64{
  struct{
    U64 mantissa : 52;
    U64 exponent : 11;
    U64 sign     : 1;
  };
  F64 f64;
};

//
// BIGNUM
//  A bare-minimums BigNum implementation, including BigZ ("big int") and BigQ ("big rational")
//  It lives in this source file because the sb_printf implementation requires this, at least
//  during development!
//

//~ First we have BigN, which is the basis for other BigNum data structures.

typedef struct BigN BigN;
struct BigN{
  U64 *limbs;
  I64 count;
};

static BigN bn_create(U64 *limbs, I64 count);

static bool bn_alias(BigN a, BigN b); // Checks if two BigN structs overlap, and returns true in that case
static BigN bn_subnumber(BigN n, I64 begin, I64 end);
static I64 bn_compare(BigN a, BigN b); // Returns `sign(a - b)`
static bool bn_equals(BigN a, BigN b);
static bool bn_equals_zero(BigN n);

// NOTE: These operations assumes that the output does not alias any of the inputs (the inputs, however, may alias with eachother)

// These two operations shifts the result down and up, respectively, and returns the resulting number of limbs.
// In the case of shift up the output is assumed to have capacity enough to store the result. This means
// having `count + (shift / 64) + 1` as the capacity.
static I64 bn_shift_down(U64 *out, I64 count, U64 shift);
static I64 bn_shift_up(U64 *out, I64 count, U64 shift);

// Assumes `out` has `a.count + 1` capacity. Assumes that `a.count >= b.count`.
static void bn_add(U64 *out, BigN a, BigN b);

// Assumes `out` has `a.count` capacity. Assumes that `a` is the larger of the two input numbers.
static void bn_sub(U64 *out, BigN a, BigN b);

// Assumes `out` has `a.count + b.count` capacity.
static I64 bn_mul(U64 *out, BigN a, BigN b);
static I64 bn_mul_shift_down(U64 *out, BigN a, BigN b, U64 shift); // Computes `(a * b) >> shift`

//~ BigN IMPLEMENTATION

static void _bn_verify_init_and_unpadded(BigN n){
  assert(n.count >= 0 && "BigN with negative count detected");
  assert(n.count > 0 && "Value is uninitialized");
  if(n.count > 1){
    assert(n.limbs[n.count - 1] != 0 && "Illegal padding with zeros detected!");
  }
}

static BigN bn_create(U64 *limbs, I64 count){
  assert(count > 0);
  BigN result = {0};
  result.limbs = limbs;
  result.count = count;
  return result;
}

static bool bn_alias(BigN a, BigN b){
  uintptr_t a_begin = (uintptr_t)a.limbs;
  uintptr_t a_end = a_begin + a.count * sizeof(U64);
  uintptr_t b_begin = (uintptr_t)b.limbs;
  uintptr_t b_end = b_begin + b.count * sizeof(U64);

  // TODO: Check if this is off by one
  return (b_begin < a_end && a_begin < b_end);
}

static BigN bn_subnumber(BigN n, I64 begin, I64 end){
  assert(end >= begin);
  assert(begin >= 0);

  BigN result = {0};

  if(n.count >= end && end > begin){
    result.limbs = n.limbs + begin;
    result.count = end - begin;
  }

  return result;
}

static I64 bn_compare(BigN a, BigN b){
  _bn_verify_init_and_unpadded(a);
  _bn_verify_init_and_unpadded(b);

  I64 result = 0;
  if(a.count > b.count){
    result = 1;
  }else if(a.count < b.count){
    result = -1;
  }else{
    for(I64 index = a.count - 1; index >= 0; index -= 1){
      if(a.limbs[index] > b.limbs[index]){
        result = 1;
        break;
      }else if(a.limbs[index] < b.limbs[index]){
        result = -1;
        break;
      }
    }
  }
  return result;
}

static bool bn_equals(BigN a, BigN b){
  _bn_verify_init_and_unpadded(a);
  _bn_verify_init_and_unpadded(b);

  bool result = false;
  if(a.count == b.count){
    I64 count = a.count;

    result = true;
    fiz(count){
      if(a.limbs[i] != b.limbs[i]){
        result = false;
        break;
      }
    }
  }
  return result;
}

static bool bn_equals_zero(BigN n){
  _bn_verify_init_and_unpadded(n);
  return (n.limbs[n.count - 1] == 0);
}

static void _bn_unpad(U64 *limbs, I64 *_count){
  I64 count = *_count;
  assert(count >= 1);
  while(count > 1 && !limbs[count - 1]){
    count -= 1;
  }
  *_count = count;
}

static I64 bn_shift_down(U64 *limbs, I64 count, U64 shift){
  U64 big_shift = (shift >> LIT_U64(6));
  U64 small_shift = (shift & LIT_U64(63));

  I64 result;
  if(count > big_shift){
    result = count - big_shift; // This then needs to be unpadded
    if(small_shift){
      fiz(count - big_shift){
        U64 high = 0;
        if(i + big_shift + 1 < count){
          high = limbs[i + big_shift + 1];
        }
        U64 low = limbs[i + big_shift + 0];

        limbs[i] = ((low >> small_shift) | (high << (LIT_U64(64) - small_shift)));
      }

      _bn_unpad(limbs, &result);
    }else{
      copy_items(limbs, limbs + big_shift, U64, count - big_shift);
    }
  }else{
    limbs[0] = 0;
    result = 1;
  }
  return result;
}

static I64 bn_shift_up(U64 *out, I64 count, U64 shift){
  U64 big_shift = (shift >> LIT_U64(6));
  U64 small_shift = (shift & LIT_U64(63));

  I64 result = count + big_shift;
  if(small_shift){
    result += 1;
    for(I64 i = count; i >= 0; i -= 1){
      U64 high = 0;
      if(i < count){
        high = out[i - 0];
      }
      U64 low = 0;
      if(i > 0){
        low = out[i - 1];
      }

      out[i + big_shift] = (high << small_shift);
      out[i + big_shift] |= (low >> (LIT_U64(64) - small_shift));
    }
    _bn_unpad(out, &result);
  }else{
    copy_items(out + big_shift, out, U64, count);
  }
  clear_items(out, U64, big_shift);
  return result;
}

static void bn_add(U64 *out, BigN a, BigN b){
  _bn_verify_init_and_unpadded(a);
  _bn_verify_init_and_unpadded(b);

  assert(a.count >= b.count); // Here we only require `a` to have a greater or equal limb count than `b`

  U64 carry = 0;
  I64 i = 0;
  for(; i < b.count; i += 1){
    U64 value;
#if COMPILER_GCC
    CT_ASSERT(sizeof(unsigned long) == sizeof(U64));
    carry = __builtin_uaddl_overflow(a.limbs[i], carry, &value);
    carry |= __builtin_uaddl_overflow(value, b.limbs[i], &value);
#elif COMPILER_MSVC
    carry = _addcarry_u64(carry, a.limbs[i], b.limbs[i], &value);
#else
#error "Unsupported compiler"
#endif

    out[i] = value;
  }
  for(; i < a.count; i += 1){
    U64 value;
#if COMPILER_GCC
    CT_ASSERT(sizeof(unsigned long) == sizeof(U64));
    carry = __builtin_uaddl_overflow(a.limbs[i], carry, &value);
#elif COMPILER_MSVC
    carry = _addcarry_u64(carry, a.limbs[i], 0, &value);
#else
#error "Unsupported compiler"
#endif
    out[i] = value;
  }
  out[i] = carry;
}

static void bn_sub(U64 *out, BigN a, BigN b){
  _bn_verify_init_and_unpadded(a);
  _bn_verify_init_and_unpadded(b);

  assert(a.count >= b.count); // In this routine we actually strictly require a to be greater than or equal to b, not just the limb count! We just don't check that in the assert for performance reasons.

  U64 carry = 0;
  I64 i = 0;
  for(; i < b.count; i += 1){
    U64 value;
#if COMPILER_GCC
    CT_ASSERT(sizeof(unsigned long) == sizeof(U64));
    carry = __builtin_usubl_overflow(a.limbs[i], carry, &value);
    carry |= __builtin_usubl_overflow(value, b.limbs[i], &value);
#else
    carry = _subborrow_u64(0, a.limbs[i], carry, &value);
    carry |= _subborrow_u64(0, value, b.limbs[i], &value);
#endif
    out[i] = value;
  }
  for(; i < a.count; i += 1){
    U64 value;
#if COMPILER_GCC
    CT_ASSERT(sizeof(unsigned long) == sizeof(U64));
    carry = __builtin_usubl_overflow(a.limbs[i], carry, &value);
#elif COMPILER_MSVC
    carry = _subborrow_u64(0, a.limbs[i], carry, &value);
#else
#error "Unsupported compiler"
#endif
    out[i] = value;
  }
  assert(!carry);
}
static I64 bn_mul(U64 *out, BigN a, BigN b){
  return bn_mul_shift_down(out, a, b, 0);
}
static void _bn_add_single_limb(U64 *out, I64 index, U64 value){
  assert(index >= 0);
  U64 carry = value;
  for(; carry; index += 1){
#if COMPILER_GCC
    CT_ASSERT(sizeof(unsigned long) == sizeof(U64));
    carry = __builtin_uaddl_overflow(out[index], carry, &out[index]);
#elif COMPILER_MSVC
    carry = _addcarry_u64(carry, out[index], 0, &out[index]);
#else
#error "Unsupported compiler"
#endif
  }
  assert(carry == 0);
}
static I64 bn_mul_shift_down(U64 *out, BigN a, BigN b, U64 shift){
  _bn_verify_init_and_unpadded(a);
  _bn_verify_init_and_unpadded(b);

  U64 big_shift = (shift >> LIT_U64(6));
  U64 small_shift = (shift & LIT_U64(63));

  clear_items(out, U64, a.count + b.count);

  // TODO: Should we do this with 32-bit muls + Karatsuba or whatever??
  fjz(a.count){
    fiz(b.count){
      if(i + j < big_shift){ continue; }

#if COMPILER_GCC
      typedef unsigned __int128 U128;
      U128 value = (U128)a.limbs[j] * (U128)b.limbs[i];
      U64 high = (U64)(value >> 64);
      U64 low = (U64)value;
#else
      U64 high;
      U64 low = _mul128(a.limbs[j], b.limbs[i], (LONG64*)&high);
#endif
      _bn_add_single_limb(out, i + j + 0 - big_shift, low);
      _bn_add_single_limb(out, i + j + 1 - big_shift, high);
    }
  }

  I64 result = a.count + b.count;
  _bn_unpad(out, &result);
  if(small_shift){
    result = bn_shift_down(out, result, small_shift);
  }
  return result;
}

//~ BigZ "BIGINT"

#define BIG_NUM_F_negative       LIT_U32(0x1)
#define BIG_NUM_F_allocated      LIT_U32(0x2) /* meaning that it is allocated and can be reallocated. Not having this flags indicates that the result is immutable. */

typedef struct BigZ BigZ;
struct BigZ{
  U32 flags; /* BIG_NUM_F_* */
  U64 *limbs;
  I64 count;
  I64 capacity; // No need to set this one for immutable BigZ's. Only relevant to BigZ's allocated (and not pushed) on an allocator.
};

// Allocator interface:
static BigZ bz_create(Allocator *allocator);
static BigZ bz_create_copy(Allocator *allocator, BigZ value);
static BigZ bz_create_i64(Allocator *allocator, I64 value);
static BigZ bz_create_u64(Allocator *allocator, U64 value);
static Allocator* bz_get_allocator(BigZ z);
static void bz_destroy(BigZ *z);

static void bz_reset(BigZ *z);

static I64 bz_sign(BigZ z);
static bool bz_negative(BigZ z);

static void bz_copy(BigZ *out, BigZ in);
static void bz_set_zero(BigZ *out);
static void bz_set_i64(BigZ *out, I64 value);
static void bz_set_u64(BigZ *out, U64 value);
static bool bz_get_i64(BigZ in, I64 *_value);
static bool bz_get_u64(BigZ in, U64 *_value);

// See sb_print_bz for printing BigZ's
static bool bz_set_base10_string(BigZ *out, String string, bool allow_negative);

static BigN bz_as_bn(BigZ z);

static bool bz_equals_zero(BigZ a);
static bool bz_equals(BigZ a, BigZ b);
static bool bz_equals_i64(BigZ a, I64 b);
#if COMPILER_GCC
static bool bz_equals_u128(BigZ a, unsigned __int128 value); // NOTE(hanna): This procedure was used for debugging my implementation of Ryu the float to string algorithm. It can probably be removed if there is a reason for that in the future.
#endif
static I64 bz_compare(BigZ a, BigZ b); // 0 if equal, 1 if `a` is greater, -1 if 'b' is greater

static bool bz_get_bit(BigZ in, U64 bit_index);
static void bz_set_bit(BigZ *z, U64 bit_index, bool value);
static I64 bz_get_bit_count(BigZ z); // Returns one past the index of the high bit i.e. the number of bits (both zeros and ones) in the number

static F64 bz_get_f64(BigZ z);
static F64 bz_get_f64_mantissa_only(BigZ z);

//~ BigZ inplace operations

static void bz_shift_down(BigZ *out, BigZ in, I64 shift); // `shift` may not be negative!
static void bz_shift_up(BigZ *out, BigZ in, I64 shift); // `shift` may not be negative!
static void bz_shift(BigZ *out, BigZ in, I64 shift);

static void bz_add(BigZ *out, BigZ a, BigZ b);
static void bz_sub(BigZ *out, BigZ a, BigZ b);
static void bz_sub_absolute(BigZ *out, BigZ a, BigZ b); // computes a - abs(b)

static void bz_mul(BigZ *out, BigZ a, BigZ b);
static void bz_mul_shift_down(BigZ *out, BigZ a, BigZ b, U64 shift); // calculates (a * b) >> shift
static void bz_fused_multiply_add(BigZ *out, BigZ a, BigZ b, BigZ c); // computes a * b + c
static void bz_pow_u64(BigZ *out, BigZ a, U64 exponent);
static void bz_div(BigZ *quotient, BigZ *remainder, BigZ a, BigZ b);

static void bz_mod(BigZ *out, BigZ a, BigZ b);
static void bz_gcd(BigZ *out, BigZ a, BigZ b);

static void bz_negate(BigZ *out, BigZ in);
static void bz_absolute(BigZ *out, BigZ in);
static void bz_max(BigZ *out, BigZ a, BigZ b);
static void bz_min(BigZ *out, BigZ a, BigZ b);

//~ BigZ "streaming" operations
// NOTE: The returned BigZ from these routines is immutable!

static BigZ bz_push_copy(Allocator *allocator, BigZ z);
static BigZ bz_push_i64(Allocator *allocator, I64 value);
static BigZ bz_push_u64(Allocator *allocator, U64 value);
static BigZ bz_push_base10_string(Allocator *allocator, String string, bool allow_negative); // Returns an unset BigZ on invalid string

static BigZ bz_push_add(Allocator *allocator, BigZ a, BigZ b);
static BigZ bz_push_sub(Allocator *allocator, BigZ a, BigZ b);
static BigZ bz_push_mul(Allocator *allocator, BigZ a, BigZ b);
static BigZ bz_push_mul_shift_down(Allocator *allocator, BigZ a, BigZ b, U64 shift); // calculates (a * b) >> shift
static BigZ bz_push_div(Allocator *allocator, BigZ a, BigZ b);
static BigZ bz_push_mod(Allocator *allocator, BigZ a, BigZ b);
static BigZ bz_push_gcd(Allocator *allocator, BigZ a, BigZ b);

//~ BigZ implementation

static void _bz_verify_valid(BigZ z){
  assert(z.count >= 0 && "BigZ with negative count detected");
  assert(z.count > 0 && "Value is uninitialized");
  if(z.count > 1){
    assert(z.limbs[z.count - 1] != 0 && "Illegal padding with zeros detected!");
  }
  if(z.count == 1 && z.limbs[0] == 0){
    assert(!(z.flags & BIG_NUM_F_negative) && "Zero cannot be negative");
  }
  if((z.flags & BIG_NUM_F_allocated)){
     Allocator *allocator = bz_get_allocator(z);
     assert(allocator);
  }
}
static bool _bz_is_same(BigZ a, BigZ b){
  bool result = false;
  if(a.limbs == b.limbs){
    assert(a.count == b.count);
    assert(a.flags == b.flags);
    result = true;
  }
  return result;
}
static void _bz_deduplicate(Allocator *temp, BigZ *out, BigZ *a){
  _bz_verify_valid(*a);
  if(_bz_is_same(*out, *a)){
    *a = bz_push_copy(temp, *a);
  }
}

static void _bz_deduplicate2(Allocator *temp, BigZ *out, BigZ *a, BigZ *b){
  _bz_deduplicate(temp, out, a);
  _bz_deduplicate(temp, out, b);
}

static void _bz_unpad(BigZ *z){
  while(z->count > 1 && z->limbs[z->count - 1] == 0){
    z->count -= 1;
  }
  if(z->count == 1 && z->limbs[0] == 0){
    z->flags &= ~BIG_NUM_F_negative;
  }
}

static BigZ bz_create(Allocator *allocator){
  BigZ result = {0};
  result.flags = BIG_NUM_F_allocated;
  result.limbs = (U64*)allocator_get_stub(allocator);
  return result;
}
static BigZ bz_create_copy(Allocator *allocator, BigZ value){
  BigZ result = bz_create(allocator);
  bz_copy(&result, value);
  return result;
}
static BigZ bz_create_i64(Allocator *allocator, I64 value){
  BigZ result = bz_create(allocator);
  bz_set_i64(&result, value);
  return result;
}
static BigZ bz_create_u64(Allocator *allocator, U64 value){
  BigZ result = bz_create(allocator);
  bz_set_u64(&result, value);
  return result;
}
static Allocator* bz_get_allocator(BigZ z){
  assert((z.flags & BIG_NUM_F_allocated));
  return get_allocator(z.limbs);
}
static void bz_destroy(BigZ *z){
  assert((z->flags & BIG_NUM_F_allocated));
  allocator_free(z->limbs, z->capacity * sizeof(U64));
}

static void bz_reset(BigZ *z){
  z->flags &= ~BIG_NUM_F_negative;
  z->count = 0;
}

static void _bz_reserve(BigZ *z, I64 capacity){
  assert((z->flags & BIG_NUM_F_allocated));

  if(z->capacity < capacity){
    if(allocator_expand(z->limbs, z->capacity * sizeof(U64), capacity * sizeof(U64))){
      z->capacity = capacity;
    }else{
      // TODO: Change the allocator interface here to limit copying perhaps?
      I64 new_capacity = MAXIMUM(z->capacity * 2, capacity);
      allocator_realloc_noclear((void**)&z->limbs, z->capacity * sizeof(U64), new_capacity * sizeof(U64), ALIGNOF_TYPE(U64));
      z->capacity = new_capacity;
    }
  }
}

static I64 bz_sign(BigZ z){
  _bz_verify_valid(z);

  if(bz_equals_zero(z)){
    return 0;
  }else if((z.flags & BIG_NUM_F_negative)){
    return -1;
  }else{
    return 1;
  }
}
static bool bz_negative(BigZ z){
  return !!(z.flags & BIG_NUM_F_negative);
}

static bool _bz_same(BigZ a, BigZ b){
  if(a.limbs == b.limbs){
    assert(a.count == b.count);
    assert(a.flags == b.flags);
    return true;
  }
  return false;
}

static void bz_copy(BigZ *out, BigZ in){
  if(!_bz_same(*out, in)){
    bz_reset(out);
    _bz_reserve(out, in.count);
    if((in.flags & BIG_NUM_F_negative)){
      out->flags |= BIG_NUM_F_negative;
    }
    out->count = in.count;
    copy_items(out->limbs, in.limbs, U64, in.count);
  }
}
static void bz_set_zero(BigZ *out){
  _bz_reserve(out, 1);
  out->flags &= ~BIG_NUM_F_negative;
  out->count = 1;
  out->limbs[0] = 0;
}
static void bz_set_i64(BigZ *out, I64 value){
  bool negative = false;
  if(value < 0){
    negative = true;
    value = -value;
  }

  bz_reset(out);
  _bz_reserve(out, 1);
  if(negative){
    out->flags |= BIG_NUM_F_negative;
  }
  out->count = 1;
  out->limbs[0] = (U64)value;
}
static void bz_set_u64(BigZ *out, U64 value){
  bz_reset(out);
  _bz_reserve(out, 1);
  out->count = 1;
  out->limbs[0] = value;
}

static bool bz_get_i64(BigZ in, I64 *_value){
  bool result = false;
  I64 value = 0;

  if(in.count == 1){
    if(!bz_negative(in)){
      if(in.limbs[0] <= INT64_MAX){
        value = (I64)in.limbs[0];
        result = true;
      }
    }else{
      if(in.limbs[0] <= (U64)INT64_MAX + 1){
        value = -(I64)in.limbs[0];
      }
    }
  }

  if(_value) *_value = value;
  return result;
}
static bool bz_get_u64(BigZ in, U64 *_value){
  bool result = false;
  U64 value = 0;
  if(in.count == 1 && !(in.flags & BIG_NUM_F_negative)){
    result = true;
    value = in.limbs[0];
  }
  if(_value) *_value = value;
  return result;
}

static bool bz_set_base10_string(BigZ *out, String string, bool allow_negative){
  assert(out->flags & BIG_NUM_F_allocated);

  bool result = false;
  if(string.size > 0){
    result = true;
    Allocator *temp = temp_begin();

    bz_set_i64(out, 0);

    bool negative = false;
    I64 index = 0;
    if(allow_negative && string.data[0] == '-'){
      negative = true;
      index += 1;
    }

    BigZ ten = bz_push_i64(temp, 10);
    BigZ digit = bz_create(temp);
    for(; index < string.size; index += 1){
      U8 c = string.data[index];
      if(!IN_RANGE('0' <=, c, <= '9')){
        result = false;
        break;
      }
      bz_set_i64(&digit, (I64)(c - '0'));
      bz_mul(out, *out, ten);
      bz_add(out, *out, digit);
    }

    if(negative){
      out->flags |= BIG_NUM_F_negative;
    }

    temp_end(&temp);
  }
  return result;
}

static BigN bz_as_bn(BigZ z){
  BigN result = bn_create(z.limbs, z.count);
  return result;
}

static bool bz_equals_zero(BigZ a){
  _bz_verify_valid(a);
  return (a.limbs[a.count - 1] == 0);
}
static bool bz_equals(BigZ a, BigZ b){
  bool result = false;
  if(bz_sign(a) == bz_sign(b)){
    result = bn_equals(bz_as_bn(a), bz_as_bn(b));
  }
  return result;
}
static bool bz_equals_i64(BigZ a, I64 b){
  bool result = false;
  if(a.count == 1){
    if(b >= 0){
      if(!(a.flags & BIG_NUM_F_negative)){
        result = (a.limbs[0] == (U64)b);
      }
    }else{
      if((a.flags & BIG_NUM_F_negative)){
        result = (a.limbs[0] == 0-(U64)b); // 0 - b because otherwise MSVC complains
      }
    }
  }
  return result;
}

#if COMPILER_GCC
static bool bz_equals_u128(BigZ a, unsigned __int128 value){
  bool result = true;
  U64 high = value >> 64;
  U64 low = (U64)value;

  if(!bz_negative(a)){
    if(a.count == 1 && high == 0){
      result = (a.limbs[0] == low);
    }else if(a.count == 2 && high != 0){
      result = (a.limbs[0] == low) && (a.limbs[1] == high);
    }
  }

  return result;
}
#endif

static I64 bz_compare(BigZ a, BigZ b){
  _bz_verify_valid(a);
  _bz_verify_valid(b);

  I64 sign_a = bz_sign(a);
  I64 sign_b = bz_sign(b);

  if(sign_a > sign_b){
    return 1;
  }else if(sign_a < sign_b){
    return -1;
  }else if(sign_a == 0){
    return 0;
  }else{
    return bn_compare(bz_as_bn(a), bz_as_bn(b)) * sign_a;
  }
}

static bool bz_get_bit(BigZ in, U64 bit_index){
  U64 big_index = (bit_index >> LIT_U64(6));
  U64 small_index = (bit_index & LIT_U64(63));

  bool result = false;
  if(big_index < in.count){
    result = ((in.limbs[big_index] >> small_index) & LIT_U64(0x1));
  }
  return result;
}
static void bz_set_bit(BigZ *z, U64 bit_index, bool value){
  U64 big_index = (bit_index >> LIT_U64(6));
  U64 small_index = (bit_index & LIT_U64(63));

  if(value){
    if(big_index >= z->count){
      _bz_reserve(z, big_index + 1);
      clear_items(z->limbs + z->count, U64, big_index + 1 - z->count);
      z->count = big_index + 1;
    }
    z->limbs[big_index] |= (LIT_U64(1) << small_index);
  }else if(big_index < z->count){
    z->limbs[big_index] &= ~(LIT_U64(1) << small_index);
    _bz_unpad(z);
  }
}

static I64 bz_get_bit_count(BigZ z){
  _bz_verify_valid(z);

  I64 result = 0;
  if(!bz_equals_zero(z)){
    result += (z.count - 1) * 64;
    result += 1 + index_of_high_bit_u64(z.limbs[z.count - 1]);
  }

  return result;
}

static U64 _bz_most_significant_u64(BigZ z){
  _bz_verify_valid(z);

  U64 result;
  if(z.count == 1){
    result = z.limbs[0];
    result <<= count_leading_zeros_u64(result);
  }else{
    U64 high = z.limbs[z.count - 1];
    U64 low = z.limbs[z.count - 2];
    U64 shift = count_leading_zeros_u64(high);
    result = (high << shift) | (low >> (LIT_U64(64) - shift));
  }

  return result;
}

static F64 bz_get_f64(BigZ z){
  IEEE754_F64 result = {0};
  if(bz_equals_i64(z, 0)){
    result.f64 = 0.0;
  }else{
    I64 wanted_exponent = bz_get_bit_count(z);
    if(wanted_exponent >= 2047 - 1022){
      result.exponent = 2047;
      result.mantissa = 0;
    }else{
      result.exponent = 1022 + bz_get_bit_count(z);
      result.mantissa = (_bz_most_significant_u64(z) >> 11);
    }
    result.sign = bz_negative(z);
  }
  return result.f64;
}
static F64 bz_get_f64_mantissa_only(BigZ z){
  IEEE754_F64 result = {0};
  if(bz_equals_zero(z)){
    result.f64 = 0.0;
  }else{
    result.exponent = 1023;
    result.mantissa = (_bz_most_significant_u64(z) >> 11);
  }
  return result.f64;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static void bz_shift_down(BigZ *out, BigZ in, I64 shift){
  assert(shift >= 0);

  if(!_bz_same(*out, in)){
    bz_copy(out, in);
  }
  bz_reset(out);
  out->count = bn_shift_down(out->limbs, in.count, shift);

  if(bz_negative(in)){
    out->flags |= BIG_NUM_F_negative;
  }
}

static void bz_shift_up(BigZ *out, BigZ in, I64 shift){
  assert(shift >= 0);

  if(!_bz_same(*out, in)){
    bz_copy(out, in);
  }
  bz_reset(out);
  _bz_reserve(out, in.count + ((shift + 63) >> 6));
  out->count = bn_shift_up(out->limbs, in.count, shift);

  if(bz_negative(in)){
    out->flags |= BIG_NUM_F_negative;
  }
}

static void bz_shift(BigZ *out, BigZ in, I64 shift){
  if(shift > 0){
    bz_shift_up(out, in, shift);
  }else{
    bz_shift_down(out, in, -shift);
  }
}

static void _bz_add(BigZ *_out, BigZ a, BigZ b, bool subtract_b){
  Allocator *temp = temp_begin();

  BigZ out = *_out;
  assert((out.flags & BIG_NUM_F_allocated));

  _bz_deduplicate2(temp, &out, &a, &b);
  bz_reset(&out);

  if(bz_negative(a) == (bz_negative(b) ^ subtract_b)){
    bool negative = bz_negative(a);
    if(a.count < b.count){
      BigZ t = a;
      a = b;
      b = t;
    }

    _bz_reserve(&out, a.count + 1);
    bn_add(out.limbs, bz_as_bn(a), bz_as_bn(b));
    out.count = a.count + 1;
    _bz_unpad(&out);

    if(negative){
      out.flags |= BIG_NUM_F_negative;
    }
  }else{
    I64 comparison = bn_compare(bz_as_bn(a), bz_as_bn(b));
    if(comparison == 0){
      bz_set_zero(&out);
    }else{
      bool negative = bz_negative(a);
      if(comparison < 0){
        negative = (bz_negative(b) ^ subtract_b);
        BigZ t = a;
        a = b;
        b = t;
      }

      _bz_reserve(&out, a.count);
      bn_sub(out.limbs, bz_as_bn(a), bz_as_bn(b));
      out.count = a.count;
      _bz_unpad(&out);
      if(negative){
        out.flags |= BIG_NUM_F_negative;
      }
    }
  }

  *_out = out;
  temp_end(&temp);
}

static void bz_add(BigZ *out, BigZ a, BigZ b){
  _bz_add(out, a, b, false);
}
static void bz_sub(BigZ *out, BigZ a, BigZ b){
  _bz_add(out, a, b, true);
}
static void bz_sub_absolute(BigZ *out, BigZ a, BigZ b){
  _bz_add(out, a, b, !bz_negative(b));
}

static void bz_mul(BigZ *out, BigZ a, BigZ b){
  bz_mul_shift_down(out, a, b, 0);
}
static void bz_mul_shift_down(BigZ *_out, BigZ a, BigZ b, U64 shift){
  Allocator *temp = temp_begin();
  BigZ out = *_out;

  _bz_deduplicate2(temp, &out, &a, &b);
  bz_reset(&out);

  _bz_reserve(&out, a.count + b.count);
  out.count = a.count + b.count;
  if((bz_negative(a) ^ bz_negative(b))){
    out.flags |= BIG_NUM_F_negative;
  }
  clear_items(out.limbs, U64, a.count + b.count);

  bn_mul_shift_down(out.limbs, bz_as_bn(a), bz_as_bn(b), shift);
  _bz_unpad(&out);

  _bz_verify_valid(out);

  *_out = out;
  temp_end(&temp);
}

static void bz_fused_multiply_add(BigZ *out, BigZ a, BigZ b, BigZ c){
  Allocator *temp = temp_begin();

  BigZ t = bz_create(temp);
  bz_mul(&t, a, b);
  bz_add(out, t, c);

  temp_end(&temp);
}

static void bz_pow_u64(BigZ *out, BigZ a, U64 exponent_init){
  if(exponent_init == 0){
    bz_set_i64(out, 1);
  }else{
    Allocator *temp = temp_begin();
    // Exponentiation by squaring! See https://en.wikipedia.org/wiki/Exponentiation_by_squaring
    //_bz_deduplicate(temp, out, &a); No need for this as we only need the value to be stored in out
    if(!_bz_same(*out, a)){
      bz_copy(out, a);
    }

    BigZ y = bz_create_u64(temp, 1);

    U64 exponent = exponent_init;
    while(exponent > 1){
      if((exponent & 1) != 0){
        bz_mul(&y, *out, y);
      }
      bz_mul(out, *out, *out);
      exponent >>= 1;
    }
    bz_mul(out, *out, y);

    temp_end(&temp);
  }
}

static void bz_div(BigZ *_quotient, BigZ *_remainder, BigZ a, BigZ b){
  Allocator *temp = temp_begin();

  assert(_quotient && "If you only want remainder you should probably invoke the mod routine instead");

  BigZ quotient = *_quotient;
  BigZ remainder;
  if(_remainder){
    remainder = *_remainder;
  }else{
    remainder = bz_create(temp);
  }

  _bz_deduplicate2(temp, &quotient, &a, &b);
  _bz_deduplicate2(temp, &remainder, &a, &b);

  bz_set_zero(&quotient);
  bz_set_zero(&remainder);

  for(I64 i = bz_get_bit_count(a) - 1; i >= 0; i -= 1){
    bz_shift_up(&remainder, remainder, 1);
    bz_set_bit(&remainder, 0, bz_get_bit(a, i));

    if(bn_compare(bz_as_bn(b), bz_as_bn(remainder)) <= 0){
      bz_sub_absolute(&remainder, remainder, b);
      bz_set_bit(&quotient, i, 1);
    }
    assert(bn_compare(bz_as_bn(b), bz_as_bn(remainder)) >= 0);
  }

  if(!bz_equals_zero(quotient) && (bz_negative(a) ^ bz_negative(b))){
    quotient.flags |= BIG_NUM_F_negative;
  }

  *_quotient = quotient;
  if(_remainder){
    *_remainder = remainder;
  }
  temp_end(&temp);
}

static void bz_mod(BigZ *out, BigZ a, BigZ b){
  // TODO: We also want to not make modulo do the remainder operation, but the actual modulo which is correct for negative values
  // @OPTIMIZATION: At some point we want to use a smarter procedure for doing this
  Allocator *temp = temp_begin();
  BigZ quotient = bz_create(temp);
  bz_div(&quotient, out, a, b);
  temp_end(&temp);
}
static void bz_gcd(BigZ *out, BigZ init_a, BigZ init_b){
  Allocator *temp = temp_begin();

  // TODO: Try minimizing copies here in this routine!
  BigZ a = bz_create_copy(temp, init_a);
  BigZ b = bz_create_copy(temp, init_b);
  BigZ t = bz_create(temp);

  while(!bz_equals_zero(b)){
    bz_copy(&t, b);
    bz_mod(&b, a, t);
    bz_copy(&a, t);
  }

  bz_copy(out, a);

  temp_end(&temp);
}

static void bz_negate(BigZ *out, BigZ in){
  bz_copy(out, in); // NOTE: This works even if `out` and `in` are the same!
  if(!bz_equals_zero(in)){
    bool sign_bit = !bz_negative(in);
    if(sign_bit){
      out->flags |= BIG_NUM_F_negative;
    }else{
      out->flags &= ~BIG_NUM_F_negative;
    }
  }
}
static void bz_absolute(BigZ *out, BigZ in){
  bz_copy(out, in);
  out->flags &= ~BIG_NUM_F_negative;
}

static void bz_max(BigZ *out, BigZ a, BigZ b){
  if(bz_compare(a, b) >= 0){
    bz_copy(out, a);
  }else{
    bz_copy(out, b);
  }
}
static void bz_min(BigZ *out, BigZ a, BigZ b){
  if(bz_compare(a, b) <= 0){
    bz_copy(out, a);
  }else{
    bz_copy(out, b);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static BigZ bz_push_copy(Allocator *allocator, BigZ z){
  BigZ result = {0};
  result.limbs = allocator_push_items_noclear(allocator, U64, z.count);
  result.count = z.count;
  copy_items(result.limbs, z.limbs, U64, z.count);
  if(bz_negative(z)){
    result.flags |= BIG_NUM_F_negative;
  }
  return result;
}
static BigZ bz_push_i64(Allocator *allocator, I64 value){
  BigZ result = {0};
  result.limbs = allocator_push_items_noclear(allocator, U64, 1);
  result.count = 1;
  if(value >= 0){
    result.limbs[0] = (U64)value;
  }else{
    result.limbs[0] = 0-(U64)value;
    result.flags |= BIG_NUM_F_negative;
  }
  return result;
}
static BigZ bz_push_u64(Allocator *allocator, U64 value){
  BigZ result = {0};
  result.limbs = allocator_push_items_noclear(allocator, U64, 1);
  result.count = 1;
  result.limbs[0] = value;
  return result;
}
static BigZ bz_push_base10_string(Allocator *allocator, String string, bool allow_negative){
  Allocator *temp = temp_begin();
  BigZ result = {0};
  BigZ z = bz_create(temp);
  if(bz_set_base10_string(&z, string, allow_negative)){
    result = bz_push_copy(allocator, z);
  }
  temp_end(&temp);
  return result;
}

// NOTE: For now we only have a very lazy implementation of these routines which invokes the allocation code but then does a copy.
static BigZ bz_push_add(Allocator *allocator, BigZ a, BigZ b){
  Allocator *temp = temp_begin();
  BigZ t = bz_create(temp);
  bz_add(&t, a, b);
  BigZ result = bz_push_copy(allocator, t);
  temp_end(&temp);
  return result;
}
static BigZ bz_push_sub(Allocator *allocator, BigZ a, BigZ b){
  Allocator *temp = temp_begin();
  BigZ t = bz_create(temp);
  bz_sub(&t, a, b);
  BigZ result = bz_push_copy(allocator, t);
  temp_end(&temp);
  return result;
}
static BigZ bz_push_mul(Allocator *allocator, BigZ a, BigZ b){
  Allocator *temp = temp_begin();
  BigZ t = bz_create(temp);
  bz_mul(&t, a, b);
  BigZ result = bz_push_copy(allocator, t);
  temp_end(&temp);
  return result;
}
static BigZ bz_push_mul_shift_down(Allocator *allocator, BigZ a, BigZ b, U64 shift){
  Allocator *temp = temp_begin();
  BigZ t = bz_create(temp);
  bz_mul_shift_down(&t, a, b, shift);
  BigZ result = bz_push_copy(allocator, t);
  temp_end(&temp);
  return result;
}
static BigZ bz_push_div(Allocator *allocator, BigZ a, BigZ b){
  Allocator *temp = temp_begin();
  BigZ t = bz_create(temp);
  bz_div(&t, NULL, a, b);
  BigZ result = bz_push_copy(allocator, t);
  temp_end(&temp);
  return result;
}
static BigZ bz_push_mod(Allocator *allocator, BigZ a, BigZ b){
  Allocator *temp = temp_begin();
  BigZ t = bz_create(temp);
  bz_mod(&t, a, b);
  BigZ result = bz_push_copy(allocator, t);
  temp_end(&temp);
  return result;
}
static BigZ bz_push_gcd(Allocator *allocator, BigZ a, BigZ b){
  Allocator *temp = temp_begin();
  BigZ t = bz_create(temp);
  bz_gcd(&t, a, b);
  BigZ result = bz_push_copy(allocator, t);
  temp_end(&temp);
  return result;
}

//~ BigQ "BIGRATIONAL"

#if 0
typedef struct BigQ BigQ;
struct BigQ{
  U32 flags; /* BIG_NUM_F_* */
  U64 *limbs;
  I64 num_count;
  I64 denom_count;
};
static BigN bq_num(BigQ q){ BigN result = { q.limbs, q.num_count }; return result; }
static BigN bq_denom(BigQ q){ BigN result = { q.limbs + q.num_count, q.denom_count }; return result; }

// TODO: Big TODO

#endif

//
// NOTE: String Builder
//

typedef struct SBChunk SBChunk;
struct SBChunk{
  SBChunk *next;
  U32 capacity;
  U32 cursor;
  U8 data[0];
};

typedef struct StringBuilder StringBuilder;
struct StringBuilder{
  // NOTE(hanna): StringBuilder has two modes:
  // 1. `allocator` is set and new chunks are allocated as we go. If the allocator runs out of space its out-of-memory handling mechanism is triggered
  // 2. `allocator` is NULL and a fixed size buffer is used. The flag `ran_out_of_space` is set upon running out of space.
  Allocator *allocator;
  SBChunk *first_chunk;
  SBChunk *last_chunk;
  I64 total_size; // Total size of the string content, i.e. sum of `cursor`s in each SBChunk if we did not run out of space. If we ran out of space this is the required amount of string storage.
  bool ran_out_of_space;
};

static StringBuilder sb_create(Allocator *allocator){
  StringBuilder result = {0};
  assert(allocator != NULL);
  result.allocator = allocator;
  return result;
}
static StringBuilder sb_create_fixed(U8 *buffer, I64 size){
  StringBuilder result = {0};
  assert(size < UINT32_MAX); // If you reach this for a real use case, then you either need to change SBChunk to handle larger things or emit multiple SBChunk's
  if(size < sizeof(SBChunk)){
    result.ran_out_of_space = true;
  }else{
    SBChunk *chunk = result.first_chunk = result.last_chunk = (SBChunk*)buffer;
    clear_item(chunk);
    chunk->capacity = size - sizeof(SBChunk);
  }
  return result;
}
static I64 sb_fixed_required_bytes(StringBuilder *sb){
  assert(sb->allocator == NULL);
  assert(sb->first_chunk == sb->last_chunk);
  return sb->total_size + sizeof(SBChunk);
}
static String sb_fixed_as_string(StringBuilder *sb){
  assert(sb->allocator == NULL);
  assert(sb->first_chunk == sb->last_chunk);
  SBChunk *chunk = sb->first_chunk;
  assert(chunk);
  assert(!sb->ran_out_of_space);

  String result = string_create(chunk->data, chunk->cursor);
  return result;
}

static String sb_to_string(StringBuilder *sb, Allocator *allocator){
  assert(sb->total_size < UINT32_MAX);

  U8 *data = allocator_push_items_noclear(allocator, U8, sb->total_size);
  size_t cursor = 0;
  for(SBChunk *chunk = sb->first_chunk; chunk; chunk = chunk->next){
    memcpy(data + cursor, chunk->data, chunk->cursor);
    cursor += chunk->cursor;
  }
  assert(cursor == sb->total_size);

  String result = {0};
  result.data = data;
  result.size = (U32)sb->total_size;
  return result;
}

static bool sb_dump_to_file(StringBuilder *sb, String path){
  bool result = false;
  OSFile file = os_open_file_output(path);
  if(file.value){
    result = true;

    I64 cursor = 0;
    bool error = false;
    for(SBChunk *chunk = sb->first_chunk; chunk; chunk = chunk->next){
      os_write_to_file(file, cursor, chunk->data, chunk->cursor, &error);
      cursor += chunk->cursor;
    }

    if(error){
      result = false;
    }
  }
  os_close_file(file);

  return result;
}

static U8* sb_append_buffer(StringBuilder *sb, size_t bytes){
  SBChunk *chunk = sb->last_chunk;

  if(!chunk || chunk->cursor + bytes > chunk->capacity){
    if(sb->allocator){
      U32 capacity = MAXIMUM(bytes, KILOBYTES(8));
      SBChunk *new_chunk = (SBChunk*)allocator_push_items_noclear(sb->allocator, U8, sizeof(SBChunk) + capacity);
      clear_item(new_chunk);
      new_chunk->capacity = capacity;
      if(chunk){
        chunk->next = new_chunk;
        sb->last_chunk = new_chunk;
      }else{
        sb->first_chunk = sb->last_chunk = new_chunk;
      }
      chunk = new_chunk;
    }else{
      sb->ran_out_of_space = true;
      chunk = NULL;
    }
  }

  U8 *result = NULL;
  if(chunk){
    result = chunk->data + chunk->cursor;
    chunk->cursor += bytes;
  }
  sb->total_size += bytes;

  return result;
}
static void sb_append_string(StringBuilder *sb, String string){
  U8 *data = sb_append_buffer(sb, string.size);
  if(data){
    memcpy(data, string.data, string.size);
  }
}

static void sb_append_u8(StringBuilder *sb, U8 value){
  U8 *data = sb_append_buffer(sb, 1);
  if(data){
    *data = value;
  }
}

static void sb_print_bz(StringBuilder *sb, BigZ z){
  Allocator *temp = temp_begin();
  _bz_verify_valid(z);

  if(bz_equals_zero(z)){
    sb_append_u8(sb, '0');
  }else{
    if(bz_negative(z)){
      sb_append_u8(sb, '-');
    }

    BigZ ten = bz_push_i64(temp, 10);

    BigZ value = bz_create_copy(temp, z);
    value.flags &= ~BIG_NUM_F_negative;
    BigZ rem = bz_create(temp);

    // @OPTIMIZATION: Could compute multiple digits at a time, could write directly to a buffer in reverse order instead of reversing as the last thing that we do
    Array(U8) reverse = array_create(U8, temp);

    while(!bz_equals_zero(value)){
      bz_div(&value, &rem, value, ten);
      I64 rem_i64;
      { bool v = bz_get_i64(rem, &rem_i64); assert(v); }
      array_push(&reverse, '0' + rem_i64);
    }

    fiz(reverse.count){
      sb_append_u8(sb, reverse.e[reverse.count - 1 - i]);
    }
  }

  temp_end(&temp);
}

static void sb_print_bz_hex(StringBuilder *sb, BigZ z, String separator){
  _bz_verify_valid(z);

  if(bz_equals_zero(z)){
    sb_append_u8(sb, '0');
  }else{
    if(bz_negative(z)){
      sb_append_u8(sb, '-');
    }

    fjz(z.count){
      if(j != 0){
        sb_append_string(sb, separator);
      }
      U64 limb = z.limbs[z.count - 1 - j];
      char *digits = (char*)"0123456789abcdef";
      fiz(16){
        U64 digit = (limb >> ((15 - i) * 4)) & 0xF;
        sb_append_u8(sb, digits[digit]);
      }
    }
  }
}

//
// Ryu float to decimal algorithm implementation
//

#if 0

#if 0 // The program that generates the Ryu tables:
static void generate_ryu_tables(StringBuilder *sb){
  Allocator *temp = temp_begin();

#if 0
  for(I64 e2 = 0; e2 < 969; e2 += 1){
    // NOTE(hanna): This approximation is fine up to and including e2 = 70776
    U32 log10_2 = LIT_U32(1292913986); // = floor(log10(2) * 2^32)
    I64 q = ((U64)e2 * (U64)log10_2) >> 32;
    q = MAXIMUM(0, q - 1);
#else
  for(I64 q = 0; q <= 290; q += 1){
#endif
    I64 B0 = 124;

    U32 log2_5 = LIT_U32(2493151307); // = floor(log2(5) * 2^30)
    I64 k = B0 + (((U64)q * (U64)log2_5) >> 30);

    BigZ two_pow_k = bz_create_i64(temp, 1);
    bz_shift_up(&two_pow_k, two_pow_k, k);

    BigZ five_pow_q = bz_create_i64(temp, 5);
    bz_pow_u64(&five_pow_q, five_pow_q, q);

    BigZ value = bz_create(temp);
    bz_div(&value, NULL, two_pow_k, five_pow_q);
    bz_add(&value, value, bz_push_i64(temp, 1));

//    I64 p = (-e2 + q + k);
#if 0
    assert(value.count == 2);
    sb_printf(sb, "{");
    sb_printf(sb, "0x%016I64x, ", value.limbs[0]);
    sb_printf(sb, "0x%016I64x",   value.limbs[1]);
    sb_printf(sb, "},");
#endif
  }

  sb_printf(sb, "\n\n");

#if 0
  for(I64 e2 = -1; e2 > -1076; e2 -= 1){
    // NOTE(hanna): This approximation is fine up to and including -e2 = XXX
    U32 log10_5 = LIT_U32(3002053309); // = floor(log10(5) * 2^32)
    I64 q = ((U64)-e2 * (U64)log10_5) >> 32;
    q = MAXIMUM(0, q - 1);

    I64 i = (-e2 - q);
#else
  for(I64 i = 1; i <= 325; i += 1){
#endif
    I64 B1 = 124;

    U32 log2_5 = LIT_U32(2493151307); // = floor(log2(5) * 2^30)
    I64 k = (((U64)i * (U64)log2_5) >> LIT_U64(30)) + 1 - B1; // NOTE(hanna - 2023-04-24): The paper uses `q` here instead of `-e2 - q` here, which is just plain wrong.

    BigZ five_pow = bz_create_i64(temp, 5);
    bz_pow_u64(&five_pow, five_pow, i);

    BigZ value = bz_create(temp);
    bz_shift(&value, five_pow, -k);
    assert(value.count == 2);

//    I64 p = (q - k);
#if 0
    sb_printf(sb, "{");
    sb_printf(sb, "0x%016I64x, ", value.limbs[0]);
    sb_printf(sb, "0x%016I64x",   value.limbs[1]);
    sb_printf(sb, "},");
#endif
  }

  sb_printf(sb, "\n\n");

  temp_end(&temp);
}
#endif

typedef struct DecimalF64 DecimalF64;
struct DecimalF64{
  // The value this represents is (-1)^(sign_bit) * (digits) * 10^(exponent)
  bool sign_bit;
  U64 digits;
  I64 exponent;
};
static bool decimal_f64_equals(DecimalF64 a, DecimalF64 b){
  return a.sign_bit == b.sign_bit
      && a.digits == b.digits
      && a.exponent == b.exponent;
}

static void decode_f64(F64 f64, bool *sign_bit, U64 *mantissa, I32 *exponent){
  /*
  Remember, IEEE754 double precision floating point numbers have the following format:
  U64 mantissa : 52;
  U64 exponent : 11;
  U64 sign     : 1;

  SEEEEEEEEEEEMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  ^    ^^                       ^^
  1    11                       52

  I avoided using a union here because GCC produced slower assembly for that.
  */
  U64 value;
  memcpy(&value, &f64, sizeof(value));

  *sign_bit = value >> LIT_U64(63);
  *mantissa = (value & LIT_U64(0xfffffffffffff));
  *exponent = (I32)((value >> 52) & LIT_U64(0x7ff));
}

static DecimalF64 f64_to_shortest_decimal(F64 f64){
  /*
    Hello there! This is my (Hanna's) implementation of the Ryu algorithm, an algorithm by Ulf Adams.
    See x/resources/Ryu_Adams2018.pdf or https://dl.acm.org/doi/pdf/10.1145/3192366.3192369 for the paper by Ulf describing the algorithm.
    Also see this talk by Ulf for a higher level description of the algorithm: https://www.youtube.com/watch?v=kw-U6smcLzk

    A note on the design here: I have tried unifying the branches on e2 >= 0 and e2 < 0 into a single if, and that actually caused the code to perform slightly worse!
    So I am keeping this way of doing stuff.

    Our approach is currently on average ~3ns slower than the official Ryu implementation on my machine (our is 0.020us, theirs is 0.017us). Hopefully we can make ours faster in the future!
  */

  DecimalF64 result = {0};

  // NOTE: These you are supposed to handle before deciding to call f64_to_decimal!!
//  assert(f64 != F64_INFINITY);
//  assert(f64 != -F64_INFINITY);
//  assert(f64 == f64);
//  assert(f64 != 0);

  /*
  Remember, IEEE754 double precision floating point numbers have the following format:
  U64 mantissa : 52;
  U64 exponent : 11;
  U64 sign     : 1;

  SEEEEEEEEEEEMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  ^    ^^                       ^^
  1    11                       52

  I avoided using a union here because GCC produced slower assembly for that.
  */
  U64 ieee754_mantissa;
  I32 ieee754_exponent;
  decode_f64(f64, &result.sign_bit, &ieee754_mantissa, &ieee754_exponent);

  // Extract the mantissa and exponent into our format. Handle both the subnormal case and the normalized case and bring them into a common format.
  U64 mf;
  I32 ef;
  if(ieee754_exponent == 0){
    // Subnormal numbers!
    mf = ieee754_mantissa;
    ef = 1 - 1023 - 52;
  }else{
    // Normalized numbers!
    mf = (LIT_U64(1) << LIT_U64(52)) | ieee754_mantissa;
    ef = ieee754_exponent - 1023 - 52;
  }
  // Eachother number has their bounds included, eachother number has not.
  bool accept_bounds = ((mf & LIT_U64(1)) == 0);

  // Compute binary base bounds: (u, v, w) * 2^(e_2).
  // Here u and w constitute the halfway points between the previous and current, and the current and next, floating point numbers, respectively.
  // The current number is `f64`, the previous number means the greatest floating point value smaller than `f64`, the next number is the smallest
  // floating point value greater than `f64`. We choose `e2` wisely so everything stays in integer land.
  //
  // The range of e2 is −1076 up to 969
  I32 e2 = ef - 2;
  U64 u = 4 * mf;
  if(ieee754_mantissa == 0 && ieee754_exponent > 1){ u -= 1; }
  else                                             { u -= 2; }
  U64 v = 4 * mf;
  U64 w = 4 * mf + 2;

  // These two constants are the maximum number of bits used to encode the multipliers used for going from
  // (u, v, w) * 2^e2 to (a, b, c) * 10^(e10 - q), all division results floored.
  U32 B0 = 124;
  U32 B1 = 124;

  // Compute number of loop iterations to skip
  U32 q;
  if(e2 >= 0){
    // NOTE(hanna): This approximation is fine up to and including e2 = 70776
    U32 log10_2 = LIT_U32(1292913986); // = floor(log10(2) * 2^32)
    q = ((U64)e2 * (U64)log10_2) >> 32;
  }else{
    // NOTE(hanna): This approximation is fine up to and including -e2 = 112815
    U32 log10_5 = LIT_U32(3002053309); // = floor(log10(5) * 2^32)
    q = ((U64)-e2 * (U64)log10_5) >> 32;
  }
  // The reason we do this is in order to get the proper value for `digit` in the main loop.
  q = q - (q > 0);

  // Compute whether the digits are all zeros.
  bool za = false;
  bool zb = false;
  bool zc = false;

  // BIG NOTE(hanna): This (figuring out if the numbers a, b, c are trailing zeros) was a big pain point when implementing the
  // algorithm because the paper says that you are supposed // to use (q - 1) here for the calculation of zb, but that is just
  // plain wrong (if you don't initialize `digit` correctly)!
  //
  // Consider the following case as an example:
  //
  // (This is me putting 6.8524392303996355e+19 into the algorithm.)
  // q = 2
  // a: 6...20 4 64
  // b: 6...24 5 60
  // c: 6...28 6 56
  //           3 21
  //           ^ ^^
  //      loop iteration
  //
  /*
  Here you would begin with cutting of two digits (q = 2), i.e. doing something equivalent to running the loop twice (iterations 1 and 2).
  Then if we use 'v % 5^(q - 1) == 0'  for zb we get zb = true. The problem that arises is that when the third digits are being cut off
  (iteration 3), the algorithm will wrongly believe that all the trailing digits of b are zeros, because `digit` is initialized to zero.
  The fix is either to use 'v % 5^q == 0' or explicitly calculate the digit value. The former is _way_ easier and less computationally
  taxing, so thats the approach that we take here.
  */
  //
  if(e2 >= 0){
    // q varies from 0 to 290 here
#if 0 // Godbolt C++ code to generate table2. The compiler emits all the relevant constants to use in the assembly.
    #include <stdint.h>

    bool d5(uint64_t num){ return num % 5 == 0; }
    bool d25(uint64_t num){ return num % 25 == 0; }
    bool d125(uint64_t num){ return num % 125 == 0; }
    bool d625(uint64_t num){ return num % 625 == 0; }
    bool d3125(uint64_t num){ return num % 3125 == 0; }
    bool d15625(uint64_t num){ return num % 15625 == 0; }
    bool d78125(uint64_t num){ return num % 78125 == 0; }
    bool d390625(uint64_t num){ return num % 390625 == 0; }
    bool d1953125(uint64_t num){ return num % 1953125 == 0; }
    bool d9765625(uint64_t num){ return num % 9765625 == 0; }
    bool d48828125(uint64_t num){ return num % 48828125 == 0; }
    bool d244140625(uint64_t num){ return num % 244140625 == 0; }
    bool d1220703125(uint64_t num){ return num % 1220703125 == 0; }
    bool d6103515625(uint64_t num){ return num % 6103515625 == 0; }
    bool d30517578125(uint64_t num){ return num % 30517578125 == 0; }
    bool d152587890625(uint64_t num){ return num % 152587890625 == 0; }
    bool d762939453125(uint64_t num){ return num % 762939453125 == 0; }
    bool d3814697265625(uint64_t num){ return num % 3814697265625 == 0; }
    bool d19073486328125(uint64_t num){ return num % 19073486328125 == 0; }
    bool d95367431640625(uint64_t num){ return num % 95367431640625 == 0; }
    bool d476837158203125(uint64_t num){ return num % 476837158203125 == 0; }
    bool d2384185791015625(uint64_t num){ return num % 2384185791015625 == 0; }
    bool d11920928955078125(uint64_t num){ return num % 11920928955078125 == 0; }
    bool d59604644775390625(uint64_t num){ return num % 59604644775390625 == 0; }
    bool d298023223876953125(uint64_t num){ return num % 298023223876953125 == 0; }
#endif
    // 192 bytes!
    static U64 table1[24] = { LIT_U64(1), LIT_U64(5), LIT_U64(25), LIT_U64(125), LIT_U64(625), LIT_U64(3125), LIT_U64(15625), LIT_U64(78125), LIT_U64(390625), LIT_U64(1953125), LIT_U64(9765625), LIT_U64(48828125), LIT_U64(244140625), LIT_U64(1220703125), LIT_U64(6103515625), LIT_U64(30517578125), LIT_U64(152587890625), LIT_U64(762939453125), LIT_U64(3814697265625), LIT_U64(19073486328125), LIT_U64(95367431640625), LIT_U64(476837158203125), LIT_U64(2384185791015625), LIT_U64(11920928955078125) };
    // 384 bytes!
    static U64 table2[24][2] = { { 0, 0 }, { LIT_I64(-3689348814741910323), LIT_I64(3689348814741910323) }, { LIT_I64(-8116567392432202711), LIT_I64(737869762948382064) }, { LIT_I64(2066035336255469781), LIT_I64(147573952589676412) }, { LIT_I64(-3276141747490816367), LIT_I64(29514790517935282) }, { LIT_I64(6723469279985657373), LIT_I64(5902958103587056) }, { LIT_I64(8723391485480952121), LIT_I64(1180591620717411) }, { LIT_I64(-1944670517645719899), LIT_I64(236118324143482) }, { LIT_I64(-4078282918271054303), LIT_I64(47223664828696) }, { LIT_I64(-8194354213138031507), LIT_I64(9444732965739) }, { LIT_I64(5739826786856214345), LIT_I64(1888946593147) }, { LIT_I64(1147965357371242869), LIT_I64(377789318629) }, { LIT_I64(3918941886216158897), LIT_I64(75557863725) }, { LIT_I64(-6594909252240588867), LIT_I64(15111572745) }, { LIT_I64(6059715779035702873), LIT_I64(3022314549) }, { LIT_I64(8590640785290961221), LIT_I64(604462910) }, { LIT_I64(-1971220657683718079), LIT_I64(120892582) }, { LIT_I64(-4083592946278653939), LIT_I64(24178517) }, { LIT_I64(-4506067403997641111), LIT_I64(4835704) }, { LIT_I64(2788135333942382101), LIT_I64(967141) }, { LIT_I64(-3131721747953433903), LIT_I64(193429) }, { LIT_I64(-8005041979074507427), LIT_I64(38686) }, { LIT_I64(5777689233668919161), LIT_I64(7738) }, { LIT_I64(-2533810968008126491), LIT_I64(1548) }, };
    if(q < 24){ // <-- I think a tighter bound here would be possible, but it doesn't seem to have any measurable effect on performance on random input, so whatever.
      // I also tried using the fact that at most one of these can be true and branch on that, but that had no impact on performance for random input.
#if 0
      // Average timing on random input: 0.021us
      za = (u % table1[q] == 0);
      zb = (v % table1[q] == 0);
      zc = (w % table1[q] == 0);
#else
      // Average timing on random input: 0.020us. So this approach is faster!
      U64 C0 = table2[q][0];
      U64 C1 = table2[q][1];
      za = (u * C0 <= C1);
      zb = (v * C0 <= C1);
      zc = (w * C0 <= C1);
#endif
    }
  }else{
    // q varies from 0 to 751 here
    if(q < 57){
      U64 mask = (LIT_U64(1) << (U64)q) - 1;
      za = ((u & mask) == 0);
      zb = ((v & mask) == 0);
      zc = ((w & mask) == 0);
    }
  }

  // Compute k
  I32 k;
  // NOTE(hanna): This approximation is fine up to and including e = 55266, where (e * log2_5) >> 30 = floor(log2(5^e))
  U32 log2_5 = LIT_U32(2493151307); // = floor(log2(5) * 2^30)
  if(e2 >= 0){
    k = B0 + (((U64)q * (U64)log2_5) >> 30);
  }else{
    // The range of -e2 - q is 1 up to and including 324
    k = (((U64)(-e2 - q) * (U64)log2_5) >> 30) + 1 - B1; // NOTE(hanna - 2023-04-24): The paper uses `q` here instead of `-e2 - q` here, which is just plain wrong.
  }

  /*
  These are precomputed values of powers of five and their inverses, used in the Ryu algorithm by Ulf Adams.
  The positive table holds 2^k / 5^q (where k is a suitable value computed from q). (This is used for positive values and non-negative values of e2.)
  The negative table holds 5^(-e2 - q) / 2^k (where k is a suitable value computed from -e2 - q). (This is used for negative values of e2.)
  The tables are generated by the program #if-ed out below.
  */
  static U64 ryu_table_pos[291][2] = { {0x0000000000000001, 0x1000000000000000},{0xcccccccccccccccd, 0x0ccccccccccccccc},{0xa3d70a3d70a3d70b, 0x0a3d70a3d70a3d70},{0xb645a1cac083126f, 0x083126e978d4fdf3},{0xbd3c36113404ea4b, 0x0d1b71758e219652},{0x30fcf80dc33721d6, 0x0a7c5ac471b47842},{0x5a63f9a49c2c1b11, 0x08637bd05af6c69b},{0xc3d32907604691b5, 0x0d6bf94d5e57a42b},{0xcfdc20d2b36ba7c4, 0x0abcc77118461cef},{0x731680a88f895304, 0x089705f4136b4a59},{0xeb573440e5a884d2, 0x0dbe6fecebdedd5b},{0xef78f69a51539d75, 0x0afebff0bcb24aaf},{0xbf93f87b7442e45e, 0x08cbccc096f5088c},{0x32865a5f206b06fc, 0x0e12e13424bb40e1},{0xf538484c19ef38ca, 0x0b424dc35095cd80},{0x90f9d37014bf60a2, 0x0901d7cf73ab0acd},{0xb4c2ebe687989a9c, 0x0e69594bec44de15},{0x909befeb9fad487d, 0x0b877aa3236a4b44},{0x73aff322e62439fd, 0x09392ee8e921d5d0},{0x52b31e9e3d06c32f, 0x0ec1e4a7db69561a},{0xa88f4bb1ca6bcf59, 0x0bce5086492111ae},{0xed3f6fc16ebca5e1, 0x0971da05074da7be},{0x15324c68b12dd634, 0x0f1c90080baf72cb},{0x775b7053c0f1782a, 0x0c16d9a0095928a2},{0x2c4926a967279355, 0x09abe14cd44753b5},{0x13a83ddbd83f5221, 0x0f79687aed3eec55},{0xa95364afe032a81a, 0x0c612062576589dd},{0x8775ea264cf55348, 0x09e74d1b791e07e4},{0xd8bca9d6e1888540, 0x0fd87b5f28300ca0},{0xe096ee45813a0434, 0x0cad2f7f5359a3b3},{0x1a1258379a94d029, 0x0a2425ff75e14fc3},{0x480eacf948770cee, 0x081ceb32c4b43fcf},{0xa67de18eda5814b0, 0x0cfb11ead453994b},{0x1ecb1ad8aeacdd59, 0x0a6274bbdd0fadd6},{0x4bd5af13bef0b114, 0x084ec3c97da624ab},{0x7955e4ec64b44e87, 0x0d4ad2dbfc3d0778},{0x2dde50bd1d5d0b9f, 0x0aa242499697392d},{0x57e50d64177da2e6, 0x0881cea14545c757},{0x596e7bd358c904a3, 0x0d9c7dced53c7225},{0x7abec975e0a0d082, 0x0ae397d8aa96c1b7},{0x62323ac4b3b3da02, 0x08b61313bbabce2c},{0x36b6c46dec52f669, 0x0df01e85f912e37a},{0xc55f038b237591ee, 0x0b267ed1940f1c61},{0x377f3608e92adb25, 0x08eb98a7a9a5b04e},{0x58cb89a7db77c507, 0x0e45c10c42a2b3b0},{0x13d607b97c5fd0d3, 0x0b6b00d69bb55c8d},{0xdcab3961304ca70f, 0x09226712162ab070},{0xfaab8f01e6e10b4b, 0x0e9d71b689dde71a},{0x95560c018580d5d6, 0x0bb127c53b17ec15},{0xddde7001379a44ab, 0x095a8637627989aa},{0xc963e66858f6d445, 0x0ef73d256a5c0f77},{0x6de98520472bdd04, 0x0bf8fdb78849a5f9},{0xbe546a8038efe403, 0x0993fe2c6d07b7fa},{0xfd53dd99f4b3066b, 0x0f53304714d9265d},{0xcaa97e14c3c26b89, 0x0c428d05aa4751e4},{0xd55464dd69685607, 0x09ced737bb6c4183},{0xeeed6e2f0f0d5672, 0x0fb158592be068d2},{0xbf245825a5a44528, 0x0c8de047564d20a8},{0x65b6aceaeae9d0ed, 0x0a0b19d2ab70e6ed},{0x1e2bbd88bbee40be, 0x0808e17555f3ebf1},{0x63792f412cb06795, 0x0cdb02555653131b},{0xb5fa8c3423c052de, 0x0a48ceaaab75a8e2},{0x91953cf68300424b, 0x083a3eeeef9153e8},{0xe8eec7f0d19a03ab, 0x0d29fe4b18e88640},{0x53f2398d747b3623, 0x0a87fea27a539e9a},{0xa98e947129fc2b4f, 0x086ccbb52ea94bae},{0x75b0ed81dcc6abb1, 0x0d7adf884aa87917},{0x5e272467e3d222f4, 0x0ac8b2d36eed2dac},{0xb1b8e9ecb641b590, 0x08a08f0f8bf0f156},{0x4f8e431456cf88e7, 0x0dcdb1b279818224},{0x72d835a9df0c6d86, 0x0b0af48ec79ace83},{0xf579c487e5a38ad1, 0x08d590723948a535},{0x225c6da63c38de1c, 0x0e2280b6c20dd523},{0x81e38aeb6360b1b0, 0x0b4ecd5f01a4aa82},{0x9b1c6f22b5e6f48d, 0x090bd77f3483bb9b},{0x2b60b1d1230b20e1, 0x0e7958cb87392c2c},{0xef808e40e8d5b3e7, 0x0b94470938fa89bc},{0xbf9a0b6720aaf653, 0x09436c0760c86e30},{0x3290123e9aab23b7, 0x0ed246723473e381},{0xf5400e987bbc1c93, 0x0bdb6b8e905cb600},{0x5dccd879fc967d42, 0x097c560ba6b0919a},{0x2fae27299423fb9d, 0x0f2d56790ab41c2a},{0xbfbe85badce99617, 0x0c24452da229b021},{0xcc986afbe3ee11ac, 0x09b69dbe1b548ce7},{0x475a44c6397ce913, 0x0f8a95fcf88747d9},{0x391503d1c79720dc, 0x0c6ede63fa05d314},{0x60dd9ca7d2df4d7d, 0x09f24b832e6b0f43},{0xce2f610c84987bfb, 0x0fea126b7d78186b},{0xa4f2b40a03ad2ffc, 0x0cbb41ef979346bc},{0xb728900802f0f330, 0x0a2f67f2dfa90563},{0xf8ed400668c0c28d, 0x0825ecc24c873782},{0x27e2000a41346a7b, 0x0d097ad07a71f26b},{0xecb4ccd500f6bb96, 0x0a6dfbd9fb8e5b88},{0x56f70a4400c562de, 0x0857fcae62d8493a},{0x24be76d3346f0496, 0x0d59944a37c0752a},{0x1d652bdc29f26a12, 0x0aae103b5fcd2a88},{0xb11dbcb0218ebb42, 0x088b402f7fd75539},{0x4e95fab368e45ecf, 0x0dab99e59958885c},{0x3ede622920b6b240, 0x0aefae51477a06b0},{0x657eb4edb3c55b66, 0x08bfbea76c619ef3},{0xd59787e2b93bc570, 0x0dff9772470297eb},{0x447939822dc96ac0, 0x0b32df8e9f354656},{0x69fa946824a12233, 0x08f57fa54c2a9eab},{0xdcc420a6a101d052, 0x0e55990879ddcaab},{0xb09ce6ebb4017375, 0x0b77ada0617e3bbc},{0xf3b0b8bc9001292a, 0x092c8ae6b464fc96},{0x52b45ac74ccea843, 0x0eadab0aba3b2dbe},{0xa890489f70a55369, 0x0bbe226efb628afe},{0x53a6a07f8d510f87, 0x0964e858c91ba265},{0x85d767327bb4e5a5, 0x0f07da27a82c3708},{0x9e45ec2862f71e1e, 0x0c06481fb9bcf8d3},{0xe504bced1bf8e4e5, 0x099ea0196163fa42},{0xd4d4617b5ff4a16e, 0x0f64335bcf065d37},{0x10a9e795e65d4df2, 0x0c5029163f384a93},{0x0d54b944b84aa4c1, 0x09d9ba7832936edc},{0x7bbac2078d443acf, 0x0fc2c3f3841f17c6},{0x2fc89b393dd02f0c, 0x0c9bcff6034c1305},{0x8ca07c2dcb0cf270, 0x0a163ff802a3426a},{0x70806357d5a3f526, 0x0811ccc668829b88},{0xe733d226229feea4, 0x0ce947a3da6a9273},{0xec2974eb4ee65883, 0x0a54394fe1eedb8f},{0xbcedf722a585139c, 0x0843610cb4bf160c},{0x94aff1d108d4ec2d, 0x0d389b4787982347},{0xdd598e40d3dd89bd, 0x0a93af6c6c79b5d2},{0xb11471cd764ad498, 0x087625f056c7c4a8},{0x4e871c7bf077ba8c, 0x0d89d64d57a60774},{0xd86c16c98d2c953d, 0x0ad4ab7112eb3929},{0x46bcdf07a423aa97, 0x08aa22c0dbef60ee},{0x0ac7cb3f6d05ddbe, 0x0ddd0467c64bce4a},{0x3bd308ff8a6b17cc, 0x0b1736b96b6fd83b},{0xfca8d3ffa1ef463d, 0x08df5efabc5979c8},{0x610e1fff697ed6c7, 0x0e3231912d5bf60e},{0x80d819992132456c, 0x0b5b5ada8aaff80b},{0x00ace1474dc1d123, 0x0915e2486ef32cd6},{0x677b020baf9c81d2, 0x0e896a0d7e51e156},{0xb92f34d62616ce42, 0x0ba121a4650e4dde},{0xfa8c2a44eb4571ce, 0x094db483840b717e},{0xf746aa07ded582e3, 0x0ee2ba6c0678b597},{0x5f6bbb397f113583, 0x0be8952338609146},{0x7f89629465a75e02, 0x0986ddb5c6b3a76b},{0x65a89dba3c3efcd0, 0x0f3e2f893dec3f12},{0x8486e494fcff30a7, 0x0c31bfa0fe5698db},{0xd06bea10ca65c085, 0x09c1661a651213e2},{0xb3dfdce7aa3c673c, 0x0f9bd690a1b68637},{0x8fe64a52ee96b8fd, 0x0c7caba6e7c5382c},{0x3feb6ea8bedefa64, 0x09fd561f1fd0f9bd},{0xffdf17746497f706, 0x0ffbbcfe994e5c61},{0x3318df905079926b, 0x0cc963fee10b7d1b},{0x5c13e60d0d2e0ebc, 0x0a3ab66580d5fdaf},{0x49a984d73dbe7230, 0x082ef85133de648c},{0x0f75a15862ca504d, 0x0d17f3b51fca3a7a},{0x72c48113823b7371, 0x0a798fc4196e952e},{0x5bd06742ce95f5f4, 0x08613fd014587758},{0x2c80a537b0efefec, 0x0d686619ba27255a},{0xf066ea92f3f32657, 0x0ab9eb47c81f5114},{0x26b8bba8c328eb79, 0x0894bc396ce5da77},{0x3df45f746b74abf4, 0x0dbac6c247d62a58},{0xfe5d1929ef908990, 0x0afbd2350644eeac},{0x31e414218c73a140, 0x08c974f738372557},{0x8306869c13ec3533, 0x0e0f218b8d25088b},{0x359ed216765690f6, 0x0b3f4e093db73a09},{0xc47f0e785eaba72b, 0x08ff71a0fe2c2e6d},{0xa0cb4a5a3112a512, 0x0e65829b3046b0af},{0xb3d5d514f40eea75, 0x0b84687c269ef3bf},{0x5cab10dd900beec4, 0x0936b9fcebb25c99},{0x6111b495b3464ad3, 0x0ebdf661791d60f5},{0xe7415d448f6b6f0f, 0x0bcb2b812db11a5d},{0x529ab103a5ef8c0c, 0x096f5600f15a7b7e},{0x1dc44e6c3cb279ad, 0x0f18899b1bc3f8ca},{0x7e36a52363c1faf1, 0x0c13a148e3032d6e},{0x982bb74f8301958d, 0x09a94dd3e8cf578b},{0x8d12bee59e68ef48, 0x0f7549530e188c12},{0xa40eff1e1853f2a0, 0x0c5dd44271ad3cdb},{0xe9a598e4e0432880, 0x09e4a9cec15763e2},{0xa908f4a166d1da67, 0x0fd442e4688bd304},{0xba6d90811f0e4852, 0x0ca9cf1d206fdc03},{0xfb8ada00e5a506a8, 0x0a21727db38cb002},{0xfc6f14cd84840554, 0x081ac1fe293d599b},{0xc7182148d4066eec, 0x0cf79cc9db955c2c},{0x9f4681071005258a, 0x0a5fb0a17c777cf0},{0xb29ecd9f40041e08, 0x084c8d4dfd2c63f3},{0xb7647c3200069672, 0x0d47487cc8470652},{0xc5e9fcf4ccd211f5, 0x0a9f6d30a038d1db},{0x9e54ca5d70a80e5e, 0x087f8a8d4cfa417c},{0x63badd624dd9b096, 0x0d98ddaee19068c7},{0xe9624ab50b148d45, 0x0ae0b158b4738705},{0xede83bc408dd3dd1, 0x08b3c113c38f9f37},{0x16405fa00e2ec94e, 0x0dec681f9f4c31f3},{0xde99e619a4f23aa5, 0x0b23867fb2a35b28},{0xe547eb47b7282eea, 0x08e938662882af53},{0xa20caba5f1d9e4aa, 0x0e41f3d6a7377eec},{0x81a3bc84c17b1d55, 0x0b67f6455292cbf0},{0x67b6306a34627ddd, 0x091ff83775423cc0},{0x72bd1a438703fc95, 0x0e998d258869facd},{0x28974836059cca11, 0x0bae0a846d219571},{0xed45d35e6ae3d4db, 0x09580869f0e7aac0},{0x486fb897116c87c4, 0x0ef340a98172aace},{0x6d262d45a78a0636, 0x0bf5cd54678eef0b},{0x5751bdd152d4d1c5, 0x0991711052d8bf3c},{0xbee92fb5515482d5, 0x0f4f1b4d515acb93},{0xcbedbfc4411068aa, 0x0c3f490aa77bd60f},{0x3cbe3303674053bc, 0x09cc3a6eec6311a6},{0xc796b805720085f9, 0x0fad2a4b13d1b5d6},{0x06122cd128006b2d, 0x0c8a883c0fdaf7df},{0x380e8a40eccd228b, 0x0a086cfcd97bf97f},{0x600ba1cd8a3db53c, 0x0806bd9714632dff},{0x667902e276c921f9, 0x0cd795be87051665},{0x852d9be85f074e61, 0x0a46116538d0deb7},{0x04247cb9e59f71e7, 0x08380dea93da4bc6},{0x6d072df63c324fd8, 0x0d267caa862a12d6},{0xbd9f57f830283fe0, 0x0a8530886b54dbde},{0xcae5dff9c020331a, 0x086a8d39ef77164b},{0x77d633293366b829, 0x0d77485cb25823ac},{0x9311c2875c522cee, 0x0ac5d37d5b79b623},{0x0f41686c49db5725, 0x089e42caaf9491b6},{0x7ecf0d7a0fc5583b, 0x0dca04777f541c56},{0xcbd8d794d96aacfc, 0x0b080392cc4349de},{0xd64712dd7abbbd96, 0x08d3360f09cf6e4b},{0xbd3e8495912c628a, 0x0e1ebce4dc7f16df},{0x30fed077a756b53b, 0x0b4bca50b065abe6},{0xf3ff0d2c85def763, 0x09096ea6f3848984},{0x5331aeada2fe589d, 0x0e757dd7ec07426e},{0x428e2557b59846e4, 0x0b913179899f6858},{0x6871b7795e136bea, 0x0940f4613ae5ed13},{0xda4f8bf563524643, 0x0ece53cec4a314eb},{0x150c6ff782a83836, 0x0bd8430bd0827723},{0xaa705992ceecf9c5, 0x0979cf3ca6cec5b5},{0x43e6f5b7b17b293a, 0x0f294b943e17a2bc},{0x6985915fc12f542f, 0x0c21094364dfb563},{0x879e0de63425dcf2, 0x09b407691d7fc44f},{0x0c30163d203c94b7, 0x0f867241c8cc6d4c},{0xa359ab6419ca1092, 0x0c6b8e9b0709f109},{0x1c47bc5014a1a6db, 0x09efa548d26e5a6e},{0x2d3f93b35435d7c5, 0x0fe5d54150b090b0},{0x8a9942f5dcf7dfd1, 0x0cb7ddcdda26da26},{0xd54768c4b0c64ca7, 0x0a2cb1717b52481e},{0x776c53d08d6b7086, 0x0823c12795db6ce5},{0xbf13b94daf124da3, 0x0d0601d8efc57b08},{0xff42faa48c0ea482, 0x0a6b34ad8c9dfc06},{0x65cf2eea09a55068, 0x0855c3be0a17fcd2},{0x6fb1e4a9a90880a7, 0x0d5605fcdcf32e1d},{0x8c8e5087ba6d33b9, 0x0aab37fd7d8f5817},{0xd6d8406c95242961, 0x0888f99797a5e012},{0x8af39a475506a89a, 0x0da7f5bf59096684},{0xd58fae9f773886e2, 0x0aecc49914078536},{0xde0c8bb2c5c6d24f, 0x08bd6a141006042b},{0x967a791e093e1d4a, 0x0dfbdcece67006ac},{0x7861fa7e6dcb4aa2, 0x0b2fe3f0b8599ef0},{0x2d1b2ecb8b090882, 0x08f31cc0937ae58d},{0x482b7e12780e7402, 0x0e51c79a85916f48},{0x6cef980ec671f668, 0x0b749faed14125d3},{0xf0bfacd89ec191ed, 0x092a1958a7675175},{0xb465e15a979c1cae, 0x0ea9c227723ee8bc},{0x29eb1aaedfb016f2, 0x0bbb01b9283253ca},{0x54bc1558b2f3458e, 0x096267c7535b763b},{0x8793555ab7eba27d, 0x0f03d93eebc589f8},{0x9fa911155fefb531, 0x0c0314325637a193},{0xb2eda7444cbfc427, 0x099c102844f94e0f},{0xb7e2a53a146606a5, 0x0f6019da07f549b2},{0x2cb550fb4384d21e, 0x0c4ce17b399107c2},{0x56f773fc3603db4b, 0x09d71ac8fada6c9b},{0x24bf1ff9f0062bab, 0x0fbe9141915d7a92},{0xea327ffb266b5623, 0x0c987434744ac874},{0x21c1fffc1ebc44e9, 0x0a139029f6a239f7},{0xe7ce66634bc9d0ba, 0x080fa687f881c7f8},{0x3fb0a3d212dc8129, 0x0ce5d73ff402d98e},{0x6626e974dbe39a88, 0x0a5178fff668ae0b},{0x1e858790afe9486d, 0x08412d9991ed5809},{0x30d5a5b44ca873e1, 0x0d3515c2831559a8},{0x2711515d0a205cb4, 0x0a90de3535aaae20},{0x85a7744a6e804a2a, 0x0873e4f75e2224e6},{0x090bed43e40076a9, 0x0d863b256369d4a4},{0x6da3243650005eed, 0x0ad1c8eab5ee43b6},{0x2482835ea666b258, 0x08a7d3eef7f1cfc5},{0xd40405643d711d59, 0x0dd95317f31c7fa1},{0xa99cd11cfdf4177a, 0x0b1442798f49ffb4},{0xbae3da7d97f6792f, 0x08dd01fad907ffc3},{0x916c90c8f323f517, 0x0e2e69915b3fff9f},{0xdabd40a0c2832a79, 0x0b58547448ffffb2},{0xe23100809b9c21fb, 0x091376c36d99995b},{0x9d1b3400f8f9cff7, 0x0e858ad248f5c22c},{0x7daf5ccd93fb0cc6, 0x0b9e08a83a5e34f0},{0x97bf7d71432f3d6b, 0x094b3a202eb1c3f3},{0xf2cbfbe86b7ec8ab, 0x0edec366b11c6cb8},{0xc23cc986bc656d56, 0x0be5691ef416bd60},{0x6830a13896b78aab, 0x09845418c345644d},{0xa6b43527578c1111, 0x0f3a20279ed56d48},{0x5229c41f793cda74, 0x0c2e801fb244576d},{0x74ee367f9430aec4, 0x09becce62836ac57},{0x54b0573286b44ad2, 0x0f97ae3d0d2446f2},{0xdd59df5b9ef6a242, 0x0c795830d75038c1}, };
  static U64 ryu_table_neg[325][2] = { {0x0000000000000000, 0x0a00000000000000},{0x0000000000000000, 0x0c80000000000000},{0x0000000000000000, 0x0fa0000000000000},{0x0000000000000000, 0x09c4000000000000},{0x0000000000000000, 0x0c35000000000000},{0x0000000000000000, 0x0f42400000000000},{0x0000000000000000, 0x0989680000000000},{0x0000000000000000, 0x0bebc20000000000},{0x0000000000000000, 0x0ee6b28000000000},{0x0000000000000000, 0x09502f9000000000},{0x0000000000000000, 0x0ba43b7400000000},{0x0000000000000000, 0x0e8d4a5100000000},{0x0000000000000000, 0x09184e72a0000000},{0x0000000000000000, 0x0b5e620f48000000},{0x0000000000000000, 0x0e35fa931a000000},{0x0000000000000000, 0x08e1bc9bf0400000},{0x0000000000000000, 0x0b1a2bc2ec500000},{0x0000000000000000, 0x0de0b6b3a7640000},{0x0000000000000000, 0x08ac7230489e8000},{0x0000000000000000, 0x0ad78ebc5ac62000},{0x0000000000000000, 0x0d8d726b7177a800},{0x0000000000000000, 0x0878678326eac900},{0x0000000000000000, 0x0a968163f0a57b40},{0x0000000000000000, 0x0d3c21bcecceda10},{0x0000000000000000, 0x084595161401484a},{0x8000000000000000, 0x0a56fa5b99019a5c},{0xa000000000000000, 0x0cecb8f27f4200f3},{0x4400000000000000, 0x0813f3978f894098},{0x5500000000000000, 0x0a18f07d736b90be},{0xea40000000000000, 0x0c9f2c9cd04674ed},{0x64d0000000000000, 0x0fc6f7c404581229},{0xdf02000000000000, 0x09dc5ada82b70b59},{0x56c2800000000000, 0x0c5371912364ce30},{0x6c73200000000000, 0x0f684df56c3e01bc},{0xc3c7f40000000000, 0x09a130b963a6c115},{0x34b9f10000000000, 0x0c097ce7bc90715b},{0x01e86d4000000000, 0x0f0bdc21abb48db2},{0x4131444800000000, 0x096769950b50d88f},{0x117d955a00000000, 0x0bc143fa4e250eb3},{0xd5dcfab080000000, 0x0eb194f8e1ae525f},{0xe5aa1cae50000000, 0x092efd1b8d0cf37b},{0xdf14a3d9e4000000, 0x0b7abc627050305a},{0x96d9ccd05d000000, 0x0e596b7b0c643c71},{0xfe4820023a200000, 0x08f7e32ce7bea5c6},{0xbdda2802c8a80000, 0x0b35dbf821ae4f38},{0xed50b2037ad20000, 0x0e0352f62a19e306},{0x54526f422cc34000, 0x08c213d9da502de4},{0x69670b12b7f41000, 0x0af298d050e4395d},{0xc3c0cdd765f11400, 0x0daf3f04651d47b4},{0xfa5880a69fb6ac80, 0x088d8762bf324cd0},{0x38eea0d047a457a0, 0x0ab0e93b6efee005},{0x872a4904598d6d88, 0x0d5d238a4abe9806},{0x147a6da2b7f86475, 0x085a36366eb71f04},{0x1999090b65f67d92, 0x0a70c3c40a64e6c5},{0x5fff4b4e3f741cf6, 0x0d0cf4b50cfe2076},{0xfbff8f10e7a8921a, 0x082818f1281ed449},{0x7aff72d52192b6a0, 0x0a321f2d7226895c},{0x99bf4f8a69f76449, 0x0cbea6f8ceb02bb3},{0x802f236d04753d5b, 0x0fee50b7025c36a0},{0x501d762422c94659, 0x09f4f2726179a224},{0x6424d3ad2b7b97ef, 0x0c722f0ef9d80aad},{0xbd2e0898765a7deb, 0x0f8ebad2b84e0d58},{0x763cc55f49f88eb2, 0x09b934c3b330c857},{0x53cbf6b71c76b25f, 0x0c2781f49ffcfa6d},{0xa8bef464e3945ef7, 0x0f316271c7fc3908},{0x697758bf0e3cbb5a, 0x097edd871cfda3a5},{0xc3d52eeed1cbea31, 0x0bde94e8e43d0c8e},{0x74ca7aaa863ee4bd, 0x0ed63a231d4c4fb2},{0x88fe8caa93e74ef6, 0x0945e455f24fb1cf},{0x6b3e2fd538e122b4, 0x0b975d6b6ee39e43},{0x460dbbca87196b61, 0x0e7d34c64a9c85d4},{0xabc8955e946fe31c, 0x090e40fbeea1d3a4},{0xd6babab6398bdbe4, 0x0b51d13aea4a488d},{0x4c696963c7eed2dd, 0x0e264589a4dcdab1},{0xcfc1e1de5cf543ca, 0x08d7eb76070a08ae},{0x83b25a55f43294bc, 0x0b0de65388cc8ada},{0x249ef0eb713f39eb, 0x0dd15fe86affad91},{0xb6e3569326c78433, 0x08a2dbf142dfcc7a},{0x649c2c37f0796540, 0x0acb92ed9397bf99},{0xbdc33745ec97be90, 0x0d7e77a8f87daf7f},{0xd69a028bb3ded71a, 0x086f0ac99b4e8daf},{0xcc40832ea0d68ce0, 0x0a8acd7c0222311b},{0xbf50a3fa490c3019, 0x0d2d80db02aabd62},{0xb792667c6da79e0f, 0x083c7088e1aab65d},{0x2577001b89118593, 0x0a4b8cab1a1563f5},{0x6ed4c0226b55e6f8, 0x0cde6fd5e09abcf2},{0x8544f8158315b05b, 0x080b05e5ac60b617},{0x6696361ae3db1c72, 0x0a0dc75f1778e39d},{0xc03bc3a19cd1e38e, 0x0c913936dd571c84},{0xf04ab48a04065c72, 0x0fb5878494ace3a5},{0xb62eb0d64283f9c7, 0x09d174b2dcec0e47},{0xa3ba5d0bd324f839, 0x0c45d1df942711d9},{0x0ca8f44ec7ee3647, 0x0f5746577930d650},{0x07e998b13cf4e1ec, 0x09968bf6abbe85f2},{0x89e3fedd8c321a67, 0x0bfc2ef456ae276e},{0x2c5cfe94ef3ea101, 0x0efb3ab16c59b14a},{0x5bba1f1d158724a1, 0x095d04aee3b80ece},{0xf2a8a6e45ae8edc9, 0x0bb445da9ca61281},{0x6f52d09d71a3293b, 0x0ea1575143cf9722},{0x8593c2626705f9c5, 0x0924d692ca61be75},{0xe6f8b2fb00c77836, 0x0b6e0c377cfa2e12},{0xa0b6dfb9c0f95644, 0x0e498f455c38b997},{0xc4724bd4189bd5ea, 0x08edf98b59a373fe},{0x758edec91ec2cb65, 0x0b2977ee300c50fe},{0x12f2967b66737e3e, 0x0df3d5e9bc0f653e},{0xcbd79e0d20082ee7, 0x08b865b215899f46},{0x7ecd8590680a3aa1, 0x0ae67f1e9aec0718},{0x9e80e6f4820cc949, 0x0da01ee641a708de},{0x23109058d147fdcd, 0x0884134fe908658b},{0xebd4b46f0599fd41, 0x0aa51823e34a7eed},{0x66c9e18ac7007c91, 0x0d4e5e2cdc1d1ea9},{0xe03e2cf6bc604ddb, 0x0850fadc09923329},{0x584db8346b786151, 0x0a6539930bf6bff4},{0x6e612641865679a6, 0x0cfe87f7cef46ff1},{0xe4fcb7e8f3f60c07, 0x081f14fae158c5f6},{0x9e3be5e330f38f09, 0x0a26da3999aef774},{0xc5cadf5bfd3072cc, 0x0cb090c8001ab551},{0x373d9732fc7c8f7f, 0x0fdcb4fa002162a6},{0xe2867e7fddcdd9af, 0x09e9f11c4014dda7},{0xdb281e1fd541501b, 0x0c646d63501a1511},{0x51f225a7ca91a422, 0x0f7d88bc24209a56},{0xf3375788de9b0695, 0x09ae757596946075},{0x70052d6b1641c83a, 0x0c1a12d2fc397893},{0x4c0678c5dbd23a49, 0x0f209787bb47d6b8},{0x2f840b7ba963646e, 0x09745eb4d50ce633},{0xfb650e5a93bc3d89, 0x0bd176620a501fbf},{0xfa3e51f138ab4ceb, 0x0ec5d3fa8ce427af},{0xfc66f336c36b1013, 0x093ba47c980e98cd},{0x7b80b0047445d418, 0x0b8a8d9bbe123f01},{0xda60dc059157491e, 0x0e6d3102ad96cec1},{0x287c89837ad68db2, 0x09043ea1ac7e4139},{0x729babe4598c311f, 0x0b454e4a179dd187},{0x4f4296dd6fef3d67, 0x0e16a1dc9d8545e9},{0xd1899e4a65f58660, 0x08ce2529e2734bb1},{0x45ec05dcff72e7f8, 0x0b01ae745b101e9e},{0xd76707543f4fa1f7, 0x0dc21a1171d42645},{0xa6a06494a791c53a, 0x0899504ae72497eb},{0x90487db9d1763689, 0x0abfa45da0edbde6},{0x345a9d2845d3c42b, 0x0d6f8d7509292d60},{0x20b8a2392ba45a9b, 0x0865b86925b9bc5c},{0x28e6cac7768d7141, 0x0a7f26836f282b73},{0xf3207d795430cd92, 0x0d1ef0244af2364f},{0xf7f44e6bd49e807b, 0x08335616aed761f1},{0x75f16206c9c6209a, 0x0a402b9c5a8d3a6e},{0x136dba887c37a8c0, 0x0cd036837130890a},{0x4c2494954da2c978, 0x0802221226be55a6},{0xdf2db9baa10b7bd6, 0x0a02aa96b06deb0f},{0xd6f92829494e5acc, 0x0c83553c5c8965d3},{0xccb772339ba1f17f, 0x0fa42a8b73abbf48},{0x7ff2a760414536ef, 0x09c69a97284b578d},{0xdfef5138519684ab, 0x0c38413cf25e2d70},{0x17eb258665fc25d6, 0x0f46518c2ef5b8cd},{0x2ef2f773ffbd97a6, 0x098bf2f79d599380},{0x3aafb550ffacfd8f, 0x0beeefb584aff860},{0x495ba2a53f983cf3, 0x0eeaaba2e5dbf678},{0x2dd945a747bf2618, 0x0952ab45cfa97a0b},{0xf94f971119aeef9e, 0x0ba756174393d88d},{0x77a37cd5601aab85, 0x0e912b9d1478ceb1},{0xeac62e055c10ab33, 0x091abb422ccb812e},{0xa577b986b314d600, 0x0b616a12b7fe617a},{0x4ed5a7e85fda0b80, 0x0e39c49765fdf9d9},{0xd14588f13be84730, 0x08e41ade9fbebc27},{0xc596eb2d8ae258fc, 0x0b1d219647ae6b31},{0x36fca5f8ed9aef3b, 0x0de469fbd99a05fe},{0xe25de7bb9480d585, 0x08aec23d680043be},{0x9af561aa79a10ae6, 0x0ada72ccc20054ae},{0x41b2ba1518094da0, 0x0d910f7ff28069da},{0x690fb44d2f05d084, 0x087aa9aff7904228},{0x8353a1607ac744a5, 0x0a99541bf57452b2},{0x242889b8997915ce, 0x0d3fa922f2d1675f},{0x769956135febada1, 0x0847c9b5d7c2e09b},{0x543fab9837e69909, 0x0a59bc234db398c2},{0xe94f967e45e03f4b, 0x0cf02b2c21207ef2},{0xd1d1be0eebac278f, 0x08161afb94b44f57},{0xc6462d92a6973173, 0x0a1ba1ba79e1632d},{0x37d7b8f7503cfdcf, 0x0ca28a291859bbf9},{0x85cda735244c3d43, 0x0fcb2cb35e702af7},{0xb3a0888136afa64a, 0x09defbf01b061ada},{0x6088aaa1845b8fdd, 0x0c56baec21c7a191},{0xb8aad549e57273d4, 0x0f6c69a72a3989f5},{0x936ac54e2f678864, 0x09a3c2087a63f639},{0xf84576a1bb416a7d, 0x0c0cb28a98fcf3c7},{0xf656d44a2a11c51d, 0x0f0fdf2d3f3c30b9},{0x39f644ae5a4b1b32, 0x0969eb7c47859e74},{0x4873d5d9f0dde1fe, 0x0bc4665b59670611},{0x9a90cb506d155a7e, 0x0eb57ff22fc0c795},{0x809a7f12442d588f, 0x09316ff75dd87cbd},{0xe0c11ed6d538aeb2, 0x0b7dcbf5354e9bec},{0x18f1668c8a86da5f, 0x0e5d3ef282a242e8},{0x0f96e017d694487b, 0x08fa475791a569d1},{0x537c981dcc395a9a, 0x0b38d92d760ec445},{0xa85bbe253f47b141, 0x0e070f78d3927556},{0x293956d7478ccec8, 0x08c469ab843b8956},{0xb387ac8d1970027b, 0x0af58416654a6bab},{0xa06997b05fcc0319, 0x0db2e51bfe9d0696},{0x2441fece3bdf81f0, 0x088fcf317f22241e},{0xad527e81cad7626c, 0x0ab3c2fddeeaad25},{0x18a71e223d8d3b07, 0x0d60b3bd56a5586f},{0x6f6872d5667844e4, 0x085c705656275745},{0xcb428f8ac016561d, 0x0a738c6bebb12d16},{0x7e13336d701beba5, 0x0d106f86e69d785c},{0xcecc002466117347, 0x082a45b450226b39},{0x427f002d7f95d019, 0x0a34d721642b0608},{0x531ec038df7b441f, 0x0cc20ce9bd35c78a},{0xe7e67047175a1527, 0x0ff290242c83396c},{0x10f0062c6e984d38, 0x09f79a169bd203e4},{0x152c07b78a3e6086, 0x0c75809c42c684dd},{0x5a7709a56ccdf8a8, 0x0f92e0c353782614},{0xb88a66076400bb69, 0x09bbcc7a142b17cc},{0xe6acff893d00ea43, 0x0c2abf989935ddbf},{0xe0583f6b8c4124d4, 0x0f356f7ebf83552f},{0xec3727a337a8b704, 0x098165af37b2153d},{0x6744f18c0592e4c5, 0x0be1bf1b059e9a8d},{0xc1162def06f79df7, 0x0eda2ee1c7064130},{0x78addcb5645ac2ba, 0x09485d4d1c63e8be},{0x16d953e2bd717369, 0x0b9a74a0637ce2ee},{0x9c8fa8db6ccdd043, 0x0e8111c87c5c1ba9},{0x01d9c9892400a22a, 0x0910ab1d4db9914a},{0x82503beb6d00cab4, 0x0b54d5e4a127f59c},{0xa2e44ae64840fd61, 0x0e2a0b5dc971f303},{0x45ceaecfed289e5d, 0x08da471a9de737e2},{0xd7425a83e872c5f4, 0x0b10d8e1456105da},{0x8d12f124e28f7771, 0x0dd50f1996b94751},{0xf82bd6b70d99aaa6, 0x08a5296ffe33cc92},{0xb636cc64d1001550, 0x0ace73cbfdc0bfb7},{0xa3c47f7e05401aa4, 0x0d8210befd30efa5},{0x865acfaec34810a7, 0x08714a775e3e95c7},{0x67f1839a741a14d0, 0x0a8d9d1535ce3b39},{0xc1ede48111209a05, 0x0d31045a8341ca07},{0xd934aed0aab46043, 0x083ea2b892091e44},{0x0f81da84d5617853, 0x0a4e4b66b68b65d6},{0x936251260ab9d668, 0x0ce1de40642e3f4b},{0x3c1d72b7c6b42601, 0x080d2ae83e9ce78f},{0x0b24cf65b8612f81, 0x0a1075a24e442173},{0xcdee033f26797b62, 0x0c94930ae1d529cf},{0xc169840ef017da3b, 0x0fb9b7cd9a4a7443},{0x58e1f289560ee864, 0x09d412e0806e88aa},{0xef1a6f2bab92a27e, 0x0c491798a08a2ad4},{0x2ae10af696774b1d, 0x0f5b5d7ec8acb58a},{0x5acca6da1e0a8ef2, 0x09991a6f3d6bf176},{0xf17fd090a58d32af, 0x0bff610b0cc6edd3},{0xeddfc4b4cef07f5b, 0x0eff394dcff8a948},{0x94abdaf101564f98, 0x095f83d0a1fb69cd},{0xf9d6d1ad41abe37f, 0x0bb764c4ca7a4440},{0x384c86189216dc5e, 0x0ea53df5fd18d551},{0xc32fd3cf5b4e49bb, 0x092746b9be2f8552},{0x73fbc8c33221dc2a, 0x0b7118682dbb66a7},{0x50fabaf3feaa5334, 0x0e4d5e82392a4051},{0xd29cb4d87f2a7400, 0x08f05b1163ba6832},{0x8743e20e9ef51101, 0x0b2c71d5bca9023f},{0x6914da9246b25541, 0x0df78e4b2bd342cf},{0xa1ad089b6c2f7548, 0x08bab8eefb6409c1},{0x0a184ac2473b529b, 0x0ae9672aba3d0c32},{0x8c9e5d72d90a2741, 0x0da3c0f568cc4f3e},{0x17e2fa67c7a65889, 0x08865899617fb187},{0xdddbb901b98feeab, 0x0aa7eebfb9df9de8},{0x1552a74227f3ea56, 0x0d51ea6fa8578563},{0xed53a88958f87275, 0x08533285c936b35d},{0x68a892abaf368f13, 0x0a67ff273b846035},{0xc2d2b7569b0432d8, 0x0d01fef10a657842},{0xb9c3b29620e29fc7, 0x08213f56a67f6b29},{0x28349f3ba91b47b8, 0x0a298f2c501f45f4},{0x3241c70a936219a7, 0x0cb3f2f764271771},{0x7ed238cd383aa011, 0x0fe0efb53d30dd4d},{0x6f4363804324a40a, 0x09ec95d1463e8a50},{0x8b143c6053edcd0d, 0x0c67bb4597ce2ce4},{0xadd94b7868e94050, 0x0f81aa16fdc1b81d},{0x8ca7cf2b4191c832, 0x09b10a4e5e991312},{0x2fd1c2f611f63a3f, 0x0c1d4ce1f63f57d7},{0xfbc633b39673c8ce, 0x0f24a01a73cf2dcc},{0x1d5be0503e085d81, 0x0976e41088617ca0},{0x24b2d8644d8a74e1, 0x0bd49d14aa79dbc8},{0x2ddf8e7d60ed1219, 0x0ec9c459d51852ba},{0x5cabb90e5c942b50, 0x093e1ab8252f33b4},{0x73d6a751f3b93624, 0x0b8da1662e7b00a1},{0xd0cc512670a783ad, 0x0e7109bfba19c0c9},{0x227fb2b80668b24c, 0x0906a617d450187e},{0xab1f9f660802dedf, 0x0b484f9dc9641e9d},{0x15e7873f8a039697, 0x0e1a63853bbd2645},{0x2db0b487b6423e1e, 0x08d07e33455637eb},{0xf91ce1a9a3d2cda6, 0x0b049dc016abc5e5},{0x77641a140cc7810f, 0x0dc5c5301c56b75f},{0xaa9e904c87fcb0a9, 0x089b9b3e11b6329b},{0x9546345fa9fbdcd4, 0x0ac2820d9623bf42},{0x3a97c177947ad409, 0x0d732290fbacaf13},{0x049ed8eabcccc485, 0x0867f59a9d4bed6c},{0x05c68f256bfff5a7, 0x0a81f301449ee8c7},{0xc73832eec6fff311, 0x0d226fc195c6a2f8},{0x7c831fd53c5ff7ea, 0x083585d8fd9c25db},{0x5ba3e7ca8b77f5e5, 0x0a42e74f3d032f52},{0xf28ce1bd2e55f35e, 0x0cd3a1230c43fb26},{0x57980d163cf5b81b, 0x080444b5e7aa7cf8},{0x6d7e105bcc332621, 0x0a0555e361951c36},{0x08dd9472bf3fefaa, 0x0c86ab5c39fa6344},{0x0b14f98f6f0feb95, 0x0fa856334878fc15},{0x26ed1bf9a569f33d, 0x09c935e00d4b9d8d},{0x70a862f80ec4700c, 0x0c3b8358109e84f0},{0x8cd27bb612758c0f, 0x0f4a642e14c6262c},{0xd8038d51cb897789, 0x098e7e9cccfbd7db},{0xce0470a63e6bd56c, 0x0bf21e44003acdd2},{0x81858ccfce06cac7, 0x0eeea5d500498147},{0xb0f37801e0c43ebc, 0x095527a5202df0cc},{0xdd30560258f54e6b, 0x0baa718e68396cff},{0xd47c6b82ef32a206, 0x0e950df20247c83f},{0xe4cdc331d57fa544, 0x091d28b7416cdd27},{0xde0133fe4adf8e95, 0x0b6472e511c81471},{0x558180fddd97723a, 0x0e3d8f9e563a198e},{0xf570f09eaa7ea764, 0x08e679c2f5e44ff8},{0x32cd2cc6551e513d, 0x0b201833b35d63f7},{0xff8077f7ea65e58d, 0x0de81e40a034bcf4},{0x1fb04afaf27faf78, 0x08b112e86420f619},{0x679c5db9af1f9b56, 0x0add57a27d29339f},{0x418375281ae7822b, 0x0d94ad8b1c738087},{0x88f2293910d0b15b, 0x087cec76f1c83054},{0xab2eb3875504ddb2, 0x0a9c2794ae3a3c69},{0x15fa60692a46151e, 0x0d433179d9c8cb84},{0x8dbc7c41ba6bcd33, 0x0849feec281d7f32},{0x312b9b522906c080, 0x0a5c7ea73224deff},{0xfd768226b34870a0, 0x0cf39e50feae16be},{0x5e6a1158300d4664, 0x081842f29f2cce37},{0x360495ae3c1097fd, 0x0a1e53af46f801c5},{0x8385bb19cb14bdfc, 0x0ca5e89b18b60236},{0x246729e03dd9ed7b, 0x0fcf62c1dee382c4},{0x96c07a2c26a8346d, 0x09e19db92b4e31ba},{0x3c7098b730524188, 0x0c5a05277621be29}, };

  // Get the multiplier from the tables
  U64 *multiplier;
  if(e2 >= 0){
    multiplier = ryu_table_pos[q];
  }else{
    multiplier = ryu_table_neg[-e2 - q - 1]; // The negative one here is because -e2 - q starts out being 1 when -e2 = 1
  }

  // Multiply by the multiplier
  U64 a64;
  U64 b64;
  U64 c64;
#if COMPILER_GCC
  {
    // TODO: Support for MSVC through mulhi or whatever intrinsics.
    typedef unsigned __int128 U128;

    // For e2 >= 0 --> p + 64 varies between 117 and 124
    // For e2 < 0  --> p + 64 varies between 117 and 121
    // In conclusion we can skip caring about the low parts of the multiplication
    U64 p = (e2 >= 0) ? (-e2 + q + k - 64) : (q - k - 64);
//    assert(117 <= p);
//    assert(p <= 124);
    U128 multiplier_hi = (U128)multiplier[1];
    U128 multiplier_lo = (U128)multiplier[0];
    a64 = (U64)((u * multiplier_hi + ((u * multiplier_lo) >> 64)) >> p);
    b64 = (U64)((v * multiplier_hi + ((v * multiplier_lo) >> 64)) >> p);
    c64 = (U64)((w * multiplier_hi + ((w * multiplier_lo) >> 64)) >> p);

    // If we don't accept bounds, we want to subtract one from c. After running the main loop for q iterations the effect of doing this only persists
    // if c only had zero digits leading up to this point.
    if(!accept_bounds && zc){
      c64 -= 1;
    }
  }
#else
#error "Currently that compiler is not supported!"
#endif

  // Preparation for the main loop
  I32 e0 = q;
  bool all_a_zero = za;
  bool all_b_zero = zb;

  bool round_down = false;
  if(all_a_zero || all_b_zero){ // The general case
    U32 digit = 0;

    // The main loop
    while(true){
      U64 a64_10 = a64 / 10;
      U32 a_digit = a64 - a64_10 * 10; // NOTE: This produces the same result as what the compiler does when optimizing i.e. dividing by multiplication and shifting first, then calculating remainder. (At least according to some simple checks with godbolt.org .)
      U64 c64_10 = c64 / 10;

      if(a64_10 >= c64_10){
        break;
      }
      U64 b64_10 = b64 / 10;
      U64 b_digit = b64 - b64_10 * 10;

      a64 = a64_10;
      b64 = b64_10;
      c64 = c64_10;

      all_b_zero &= (digit == 0);
      digit = b_digit;
      all_a_zero &= (a_digit == 0);
      e0 += 1;
    }

    // If we allow going to the lower bound, strip all the zeros whenever possible!
    if(accept_bounds && all_a_zero){
      while(true){
        U64 a64_10 = a64 / 10;
        U64 a_digit = a64 - a64_10 * 10;
        if(a_digit != 0){
          break;
        }
        U64 b64_10 = b64 / 10;
        U64 b_digit = b64 - b64_10 * 10;
        U64 c64_10 = c64 / 10;

        a64 = a64_10;
        b64 = b64_10;
        c64 = c64_10;

        all_b_zero &= (digit == 0);
        digit = b_digit;

        e0 += 1;
      }
    }

    // Rounding logic (round to even)
    bool want_round_down = (digit < 5) || (digit == 5 && all_b_zero && (b64 & LIT_U64(1)) == 0);
    // If we want to round down, then see if we can do that. Also check if we are not allowed to round up.
    round_down = (want_round_down && (a64 != b64 || all_a_zero)) || (b64 == c64);
  }else{ // Fast-path for when we don't have trailing zeros.
    U32 digit = 0;

    // NOTE(hanna): The approach below is what I found to be the fastest for _random_ input.
/*
    Yes, this is kinda stupid comparing a ratio, but the denominator is _their_ performance, so this should be fine.
    I should've used time instead I know! But thats not what I did...

    loop 100
    loop 10
    Average slowdown: 1.929x

    loop 1000
    loop 100
    loop 10
    Average slowdown: 1.963x

    loop 1000
    loop 10
    Average slowdown: 1.982x

    try 100
    loop 10
    Average slowdown: 1.798x
    Average slowdown: 1.806x

    try 1000
    loop 10
    Average slowdown: 1.875x

    =========================

    try 100
    loop 10
    Average slowdown: 1.804x
    Average ours    : 0.030us
    Average theirs  : 0.017us

    try 100
    loop 10 unroll x2
    Average slowdown: 1.808x
    Average ours    : 0.030us
    Average theirs  : 0.017us

    Manually unrolling the loop x3 only made performance worse.

    ==========================

    Added a fast-path for when we go to having 32 bit values.
    This should help with numbers with a short representation, but shouldn't have any big effects on your average random input.
    Measurements, however, indicate that this doesn't seem to have any positive effect on performance (on inputs with small string representations),
    so I removed the code.

    ============================

    I really wanted to vectorize this code, but couldn't really figure any way of doing so unfortunately, as integer divisions are not easy to perform in vector registers.
    What I would want is essentially to put (a64/10, a64/100, a64/1000, a64/10000) in a vector register. Perhaps the segmenting approach in "Ryu revisited" could be of help here??

    As it currently stands (2023-05-01) we are 1.2x slower than the official Ryu implementation, which I guess is fine but not amazing.
    TODO: My plan at the moment is to use the segmented approach with vector registers and see if that is fast for finding the shortest representation.
*/
    { // Run two fused loop iterations, if possible, before entering the main loop, for performance reasons.
      U64 a64_100 = a64 / 100;
      U64 c64_100 = c64 / 100;

      if(a64_100 < c64_100){
        U64 b64_100 = b64 / 100;
        U32 b_digit = (b64 / 10 - b64_100 * 10);

        a64 = a64_100;
        b64 = b64_100;
        c64 = c64_100;
        digit = b_digit;

        e0 += 2;
      }
    }

    // The main loop
    while(true){
      U64 a64_10 = a64 / 10;
      U64 c64_10 = c64 / 10;

      if(a64_10 >= c64_10){
        break;
      }
      U64 b64_10 = b64 / 10;
      U32 b_digit = b64 - b64_10 * 10;

      a64 = a64_10;
      b64 = b64_10;
      c64 = c64_10;
      digit = b_digit;

      e0 += 1;
    }

    // Rounding logic (round to even)
    bool want_round_down = (digit < 5);
    // If we want to round down, then see if we can do that. Also check if we are not allowed to round up.
    round_down = (want_round_down && a64 != b64) || (b64 == c64);
  }

  // Compute decimal base bounds exponent: (a, b, c) * 10^(e_10)
  I32 e10;
  if(e2 >= 0){
    e10 = 0;
  }else{
    e10 = e2;
  }

  result.digits = b64 + !round_down;
  result.exponent = e0 + e10;

  return result;
}

#if 0
static DecimalF64 f64_to_decimal_naive(F64 f64){
  Allocator *temp = temp_begin();
  // NOTE: See x/resources/Ryu_Adams2018.pdf or https://dl.acm.org/doi/pdf/10.1145/3192366.3192369 for details about the algorithm.
  // https://www.youtube.com/watch?v=kw-U6smcLzk
  // This is an implementation of the naive version (section 2 of the paper), which does all the main calculations using bigints

  DecimalF64 result = {0};

  assert(f64 != F64_INFINITY);
  assert(f64 != -F64_INFINITY);
  assert(f64 == f64);

  IEEE754_F64 value;
  value.f64 = f64;

  result.sign_bit = value.sign;

  U64 mf;
  I64 ef;
  if(value.exponent == 0){
    // Subnormal numbers!
    mf = value.mantissa;
    ef = 1 - 1023 - 52;
  }else{
    // Normalized numbers!
    mf = (LIT_U64(1) << LIT_U64(52)) | (U64)value.mantissa;
    ef = value.exponent - 1023 - 52;
  }

  // Compute binary base bounds: (u, v, w) * 2^(e_2).
  // Here u and w constitute the halfway points between the previous and current, and the current and next, floating point numbers, respectively.
  // The current number is `f64`, the previous number means the greatest floating point value smaller than `f64`, the next number is the smallest
  // floating point value greater than `f64`. We choose `e2` wisely so everything stays in integer land.
  //
  // The range of e2 is −1076 up to 969
  I64 e2 = ef - 2;
  U64 u = 4 * mf;
  if(value.mantissa == 0 && value.exponent > 1){ u -= 1; }
  else                                         { u -= 2; }
  U64 v = 4 * mf;
  U64 w = 4 * mf + 2;

  // Some constants we will need later on
  BigZ five = bz_push_i64(temp, 5);
  BigZ ten = bz_push_i64(temp, 10);
  BigZ one = bz_push_i64(temp, 1);

  // Compute decimal base bounds: (a, b, c) * 2^(e_10)
  I64 e10;
  BigZ aN = bz_create(temp);
  BigZ bN = bz_create(temp);
  BigZ cN = bz_create(temp);
  if(e2 >= 0){
    e10 = 0;
    bz_shift_up(&aN, bz_push_u64(temp, u), e2);
    bz_shift_up(&bN, bz_push_u64(temp, v), e2);
    bz_shift_up(&cN, bz_push_u64(temp, w), e2);
  }else{
    e10 = e2;
    BigZ t = bz_create(temp);
    bz_pow_u64(&t, five, -e2);

    bz_mul(&aN, t, bz_push_u64(temp, u));
    bz_mul(&bN, t, bz_push_u64(temp, v));
    bz_mul(&cN, t, bz_push_u64(temp, w));
  }

  // Constants depending on the float data type, these are for F64.
  // These two constants are the maximum number of bits used to encode the multipliers used for going from
  // (u, v, w) * 2^e2 to (a, b, c) * 10^(e10 - q), all division results floored.
  I64 B0 = 124;
  I64 B1 = 124;

  // Depending on rounding mode and such:
  bool accept_smaller = false;
  bool accept_larger = false;
  bool break_tie_down = false;

  if(!accept_larger){
    bz_sub(&cN, cN, one);
  }

  // Preparation for the main loop
  I64 e0 = 0;
  bool all_a_zero = true;
  bool all_b_zero = true;

  BigZ digit = bz_create_u64(temp, 0);
  BigZ a_rem = bz_create(temp);
  BigZ a0 = bz_create(temp);
  BigZ c0 = bz_create(temp);

  // The main loop
  while(true){
    bz_div(&a0, &a_rem, aN, ten);
    bz_div(&c0, NULL, cN, ten);
    if(bz_compare(a0, c0) >= 0){
      break;
    }
    bz_copy(&aN, a0);
    all_b_zero &= bz_equals_zero(digit);
    bz_div(&bN, &digit, bN, ten);
    bz_copy(&cN, c0);
    all_a_zero &= bz_equals_zero(a_rem);
    e0 += 1;
  }

  // Strip some extra digits because we can
  if(accept_smaller && all_a_zero){
    while(true){
      bz_div(&a0, &a_rem, aN, ten);
      if(!bz_equals_zero(a_rem)){
        break;
      }
      bz_copy(&aN, a0);
      all_b_zero &= bz_equals_zero(digit);
      bz_div(&bN, &digit, bN, ten);
      bz_div(&cN, NULL, cN, ten);
      e0 += 1;
    }
  }

  // Rounding logic
  bool is_tie = bz_equals_i64(digit, 5) && all_b_zero;
  bool want_round_down = (bz_compare(digit, five) < 0) || (is_tie && break_tie_down);
  bool round_down = (want_round_down && (bz_compare(aN, bN) != 0 || all_a_zero)) || bz_compare(bN, cN) >= 0;
  if(!round_down){
    bz_add(&bN, bN, one);
  }

  result.round_down = round_down;
  result.exponent = e0 + e10;
  { bool value = bz_get_u64(bN, &result.digits); assert(value); }

  temp_end(&temp);

  return result;
}
#endif

#endif

/*
TODO(hanna): This is a macro for while we are still transitioning to the new printf implementation. Hopefully we can switch completly soon! 2023-06-10
*/
#define HANNA_UTIL_ENABLE_CUSTOM_PRINTF 0

#if HANNA_UTIL_ENABLE_CUSTOM_PRINTF
//
// Custom printf implementation
//

// BIG TODO: We should have a C parser go in and check that usages of this is fine

static void sb_append_string_padded_len(StringBuilder *sb, String string, I64 string_length, I64 min_width, bool left, U8 padding_char){
  I64 n_padding = MAXIMUM(0, min_width - string_length);

  if(left){
    sb_append_string(sb, string);
  }

  {
    U8 *buf = sb_append_buffer(sb, n_padding);
    if(buf){
      memset(buf, padding_char, n_padding);
    }
  }

  if(!left){
    sb_append_string(sb, string);
  }
}

static void sb_append_string_padded(StringBuilder *sb, String string, I64 min_width, bool left, U8 padding_char){
  I64 string_length = 0;

  U8 *string_at = string.data;
  U8 *string_end = string.data + string.size;

#if ARCH_X86_64 // It is safe to assume we have SSE if we are targetting x64
  // The approach here is to count the number of non UTF-8 continuation bytes
  U8 cont_mask = 0xc0;
  U8 cont_pattern = 0x80;

  while(((uintptr_t)(string_end - string_at) & 0xf) != 0 && string_at < string_end){
    U8 octet = string_at[0];
    if((octet & cont_mask) != cont_pattern){
      string_length += 1;
    }
    string_at += 1;
  }

  while(string_at < string_end){
    __m128i bytes_16x8 = _mm_loadu_si128((__m128i*)string_at);
    __m128i cont_mask_16x8 = _mm_set1_epi8(cont_mask);
    __m128i cont_pattern_16x8 = _mm_set1_epi8(cont_pattern);

    bytes_16x8 = _mm_and_si128(bytes_16x8, cont_mask_16x8);

    U32 bits = _mm_movemask_epi8(_mm_cmpeq_epi8(bytes_16x8, cont_pattern_16x8));
    U32 cont_count = __builtin_popcount(bits); // TODO: MSVC support

    string_length += 16 - cont_count;
    string_at += 16;
  }
  assert(string_at == string_end);
#else
  for(UTF8Iterator iter = iterate_utf8(string);
    iter.valid;
    advance_utf8_iterator(&iter))
  {
    string_length += 1;
  }
#endif

  sb_append_string_padded_len(sb, string, string_length, min_width, left, padding_char);
}

static I64 _format_parse_int(U8 **_format_at, U8 *format_end, va_list arg_list){
  U8 *format_at = *_format_at;
  I64 result = 0;

  if(format_at < format_end){
    if('1' <= format_at[0] && format_at[0] <= '9'){
      // Yes, we don't check for bounds here. The user is assumed to pass a valid format string.
      do{
        result = result * 10 + (format_at[0] - '0');
        format_at += 1;
      }while('0' <= format_at[0] && format_at[0] <= '9');
    }else if(format_at[0] == '*'){
      format_at += 1;
      result = va_arg(arg_list, int);
    }
  }

  *_format_at = format_at;
  return result;
}

static U64 _format_get_unsigned_arg(I64 byte_width, va_list arg_list){
  U64 result;

#if 1
  if(byte_width == 8){ result = va_arg(arg_list, U64); }
  else{ result = va_arg(arg_list, U32); }
#else
  switch(byte_width){
    case -1:{ result = va_arg(arg_list, U32); }break;
    case 1:{ result = va_arg(arg_list, U32); }break;
    case 2:{ result = va_arg(arg_list, U32); }break;
    case 4:{ result = va_arg(arg_list, U32); }break;
    case 8:{ result = va_arg(arg_list, U64); }break;
    default: panic("Probably a bug: unknown byte width value");
  }
#endif

  return result;
}
static U64 _format_get_signed_arg(I64 byte_width, va_list arg_list){
  I64 result;

  // TODO: Undo the evils of implicit type conversions here
#if 1
  if(byte_width == 8){ result = va_arg(arg_list, I64); }
  else{ result = va_arg(arg_list, I32); }
#else
  switch(byte_width){
    case -1:{ result = va_arg(arg_list, I32); }break;
    case 1:{ result = va_arg(arg_list, I32); }break;
    case 2:{ result = va_arg(arg_list, I32); }break;
    case 4:{ result = va_arg(arg_list, I32); }break;
    case 8:{ result = va_arg(arg_list, I64); }break;
    default: panic("Probably a bug: unknown byte width value");
  }
#endif

  return result;
}

#define FORMAT_F_alternative_form         0x1 // #
#define FORMAT_F_zero_pad                 0x2 // 0
#define FORMAT_F_left_adjust              0x4 // -
#define FORMAT_F_blank_positives          0x8 // (space)
#define FORMAT_F_plus                     0x10 // +
#define FORMAT_F_commas                   0x20 // the ' character
#define FORMAT_F_order_of_magnitudes      0x40 // $ /* TODO: Implement all features of stbsp */
#define FORMAT_F_order_of_magnitudes_byte 0x80 // $$

static void _format_print_number_with_padding(StringBuilder *sb, EnumU32(FORMAT_F_xxx) flags, String string, String prefix, String suffix, I64 field_width){
  bool left = !!(flags & FORMAT_F_left_adjust);
  U8 padding_char = ' ';
  if(!left && (flags & FORMAT_F_zero_pad)){
    padding_char = '0';
  }

  I64 n_padding = MAXIMUM(0, field_width - string.size - prefix.size);

  if(padding_char == '0'){
    sb_append_string(sb, prefix);
  }
  if(left){
    sb_append_string(sb, string);
  }

  U8 *padding = sb_append_buffer(sb, n_padding);
  if(padding){ memset(padding, padding_char, n_padding); }

  if(padding_char == ' '){
    sb_append_string(sb, prefix);
  }

  if(!left){
    sb_append_string(sb, string);
  }
}

static void _format_nonzero_integer_decimal(U8 *buffer_end, I64 *_cursor, U64 digits){
  I64 cursor = *_cursor;

  assert(digits);

#if 0 // This didn't give better performance, so its #if-ed out
  // 3kb
  U8 *digits1000 = (U8*)"000001002003004005006007008009010011012013014015016017018019020021022023024025026027028029030031032033034035036037038039040041042043044045046047048049050051052053054055056057058059060061062063064065066067068069070071072073074075076077078079080081082083084085086087088089090091092093094095096097098099100101102103104105106107108109110111112113114115116117118119120121122123124125126127128129130131132133134135136137138139140141142143144145146147148149150151152153154155156157158159160161162163164165166167168169170171172173174175176177178179180181182183184185186187188189190191192193194195196197198199200201202203204205206207208209210211212213214215216217218219220221222223224225226227228229230231232233234235236237238239240241242243244245246247248249250251252253254255256257258259260261262263264265266267268269270271272273274275276277278279280281282283284285286287288289290291292293294295296297298299300301302303304305306307308309310311312313314315316317318319320321322323324325326327328329330331332333334335336337338339340341342343344345346347348349350351352353354355356357358359360361362363364365366367368369370371372373374375376377378379380381382383384385386387388389390391392393394395396397398399400401402403404405406407408409410411412413414415416417418419420421422423424425426427428429430431432433434435436437438439440441442443444445446447448449450451452453454455456457458459460461462463464465466467468469470471472473474475476477478479480481482483484485486487488489490491492493494495496497498499500501502503504505506507508509510511512513514515516517518519520521522523524525526527528529530531532533534535536537538539540541542543544545546547548549550551552553554555556557558559560561562563564565566567568569570571572573574575576577578579580581582583584585586587588589590591592593594595596597598599600601602603604605606607608609610611612613614615616617618619620621622623624625626627628629630631632633634635636637638639640641642643644645646647648649650651652653654655656657658659660661662663664665666667668669670671672673674675676677678679680681682683684685686687688689690691692693694695696697698699700701702703704705706707708709710711712713714715716717718719720721722723724725726727728729730731732733734735736737738739740741742743744745746747748749750751752753754755756757758759760761762763764765766767768769770771772773774775776777778779780781782783784785786787788789790791792793794795796797798799800801802803804805806807808809810811812813814815816817818819820821822823824825826827828829830831832833834835836837838839840841842843844845846847848849850851852853854855856857858859860861862863864865866867868869870871872873874875876877878879880881882883884885886887888889890891892893894895896897898899900901902903904905906907908909910911912913914915916917918919920921922923924925926927928929930931932933934935936937938939940941942943944945946947948949950951952953954955956957958959960961962963964965966967968969970971972973974975976977978979980981982983984985986987988989990991992993994995996997998999";
  while(digits >= 100){
    U64 digits_1000 = digits / 1000;
    U64 three_digits = digits - digits_1000 * 1000;
    digits = digits_1000;

    buffer_end[- 3 - cursor] = digits1000[three_digits * 3 + 0];
    buffer_end[- 2 - cursor] = digits1000[three_digits * 3 + 1];
    buffer_end[- 1 - cursor] = digits1000[three_digits * 3 + 2];
    cursor += 3;
  }
#endif

  // 201 bytes
  U16 *digits100 = (U16*)"00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";
  while(digits >= 10){
    U64 digits_100 = digits / 100;
    U64 two_digits = digits - digits_100 * 100;
    digits = digits_100;

    *(U16*)&buffer_end[- 2 - cursor] = digits100[two_digits];
    cursor += 2;
  }

  if(digits > 0){
    buffer_end[- 1 - cursor] = '0' + digits;
    cursor += 1;
  }

  *_cursor = cursor;
}

/*
NOTES(2023-05-22):

Entry count is 18594

=========================
Maximum bit_count was 148
=========================
*/

static U64 _seg_ryu_pos_table[] = {};

static U64 _seg_ryu_pack_nonegative_e_and_i(I64 e, I64 i){
  assert(i >= 0);
  assert(i <= 35);
  assert(e >= 0);
  assert(e <= 971);
  return ((U64)i << LIT_U64(20)) | ((U64)e << LIT_U64(26));
#define _SEG_RYU_NONNEGATIVE_PACKED_MASK LIT_U64(0xFFFFFFFFFFF00000)
}
static U64* _seg_ryu_get_nonnegative_table_entry(I64 e, I64 i){
  U64 packed = _seg_ryu_pack_nonegative_e_and_i(e, i);

  U64 low = 0;
  U64 high = ARRAY_COUNT(_seg_ryu_pos_table);

  // TODO: If this turns out to be slow, I have two ideas:
  // --> 1. Something akin to "triangular indexing", like I did in IPS Student Challenge
  // --> 2. Have some kind of map table, which can be used for looking up stuff.

  while(true){
    U64 index = ((low + high) >> 1);

    U64 test_packed = (_seg_ryu_pos_table[index * 3 + 2] & _SEG_RYU_NONNEGATIVE_PACKED_MASK);

    if(packed == test_packed){
      return &_seg_ryu_pos_table[index * 3];
    }

    if(test_packed < packed){
      assert(high != index); // TODO: Remove these asserts for performance reasons.
      high = index;
    }else{
      assert(low != index); // TODO: Remove these asserts for performance reasons.
      low = index;
    }
  }
}

static I64 _seg_ryu_estimate_first_integer_segment(I64 exponent){
  I64 floor_log2_1000000000 = 29;
  return (exponent + 52) / floor_log2_1000000000 + 1;
}
static void _seg_ryu_append_9_digits(U8 *buffer, U32 digits){
  // TODO: Optimize! Perhaps split into two subproblems which can be solved entirely within 32 bits, instead of this which requires going to 64 bit in the machine code for performing the division.
  fiz(9){
    U32 digits_10 = digits / 10;
    U32 digit = digits - 10 * digits_10;
    digits = digits_10;

    buffer[9 - 1 - i] = '0' + digit;
  }
}

// Here `buffer` is assumed to be big enough to store the result. This means `buffer` should be quite large! TODO: Exactly how large in the general case????
static void _format_print_f64_with_f_formatting(U8 *buffer, I64 *_cursor, F64 f64){
  I64 cursor = *_cursor;

  //
  // Hello there! This is my implementation of the Ryu Printf %f format algorithm, an algorithm designed by Ulf Adams.
  // The paper by Ulf describing the algorithm can be found in x/resources/Ryu-Revisited_Adams2019.pdf or on the web on https://dl.acm.org/doi/pdf/10.1145/3360595
  //

  /*
TODO:
[ ] Implement the basic algorithm.
--> [x] basic minmax_euclid
--> [x] minmax euclid for everything
--> [ ] Tables
[ ] Do right-to-left text generation
[ ] Do everything with SIMD
  */

  bool sign_bit;
  U64 ieee754_mantissa;
  I32 ieee754_exponent;
  decode_f64(f64, &sign_bit, &ieee754_mantissa, &ieee754_exponent);

  // TODO: @COPYPASTA: This is duplicaed in f64_to_shortest_decimal. Compress?!
  // Extract the mantissa and exponent into our format. Handle both the subnormal case and the normalized case and bring them into a common format.
  U64 mf;
  I32 ef;
  if(ieee754_exponent == 0){
   // Subnormal numbers!
   mf = ieee754_mantissa;
   ef = 1 - 1023 - 52;
  }else{
   // Normalized numbers!
   mf = (LIT_U64(1) << LIT_U64(52)) | ieee754_mantissa;
   ef = ieee754_exponent - 1023 - 52;
  }

  if(sign_bit){
    buffer[cursor] = '-';
    cursor += 1;
  }

  // TODO: Handle precision and whatnot, zero padding specifically

#if 0
  I64 c0 = 118;

  I64 i = _seg_ryu_estimate_first_integer_segment(ef);
  for(; i >= 0; i -= 1){
    // TODO: The paper says to use c1 here, but that doesn't make sense. It is probably c1??
    U32 digits = mul_shift_mod(mf, _seg_ryu_get_nonnegative_table_entry(ef, i), ef - c0);
    if(digits){
      panic("TODO: nocheckin print available digits?");
    }
  }
  if(i == -1){
    buffer[cursor] = '0';
    cursor += 1;
  }else{
    for(; i >= 0; i -= 1){
      U32 digits = mul_shift_mod(mf, _seg_ryu_get_nonnegative_table_entry(ef, i), ef - c0);

      panic("TODO nocheckin append the digits");
    }
  }

  // TODO: Handle fractional part
#endif

  *_cursor = cursor;
}

static void _format_print_f64(StringBuilder *sb, F64 f64, EnumU32(FORMAT_F_xxx) flags, I64 field_width, I64 precision, char conversion_specifier, String prefix, String suffix){
  U8 buffer[256];
  if(f64 == F64_INFINITY){
    // TODO: Handle the special cases properly!
  }else if(f64 == -F64_INFINITY){
    // TODO: Handle the special cases properly!
  }else if(f64 != f64){
    // TODO: Handle the special cases properly!
  }else if(f64 == +0){
    // TODO: Handle this special case!
  }else if(f64 == -0){
    // TODO: Handle this special case!
  }else{
    /*
    TODO:
    [x] 'e', 'E' specifiers
    [ ] 'f', 'F' specifiers
    [ ] 'g', 'G' specifiers
    [ ] 'a', 'A' specifiers (hex doubles)
    */

    DecimalF64 decimal = f64_to_shortest_decimal(f64);

    U8 digits_buf[32]; // the number of digits in 2^64 (18446744073709551616) is 20, so this should be safe
    I64 digits_cursor = 0;
    assert(decimal.digits);
    _format_nonzero_integer_decimal(digits_buf + sizeof(digits_buf), &digits_cursor, decimal.digits);
    String digits_string = string_create(digits_buf + ARRAY_COUNT(digits_buf) - digits_cursor, digits_cursor);

    bool scientific = false;

    if(conversion_specifier == 'e' || conversion_specifier == 'E'){
      scientific = true;
    }else if(conversion_specifier == 'f' || conversion_specifier == 'F'){
      scientific = false;
    }else if(conversion_specifier == 'g' || conversion_specifier == 'G'){
      scientific = true; // TODO: Determine which is approximately a shorter representation
    }else{
      panic("Got passed an unknown conversion specifier");
    }
    if(scientific){
      U8 buf[32];
      I64 cursor = 0;

      // TODO: This is some code duplication. Should we factor this into some routine?
      I64 exponent = decimal.exponent + digits_string.size - 1;
      bool exponent_sign_bit = false;
      if(exponent < 0){
        exponent_sign_bit = true;
        exponent = -exponent;
      }
      if(exponent == 0){
        buf[ARRAY_COUNT(buf) - 1] = '0';
        buf[ARRAY_COUNT(buf) - 2] = '0';
        cursor += 2;
      }else{
        _format_nonzero_integer_decimal(buf + sizeof(buf), &cursor, exponent);

        if(cursor < 2){
          buf[ARRAY_COUNT(buf) - 1 - cursor] = '0';
          cursor += 1;
        }
      }

      buf[ARRAY_COUNT(buf) - 1 - cursor] = exponent_sign_bit ? '-' : '+';
      cursor += 1;

      bool uppercase = (conversion_specifier == 'E' || conversion_specifier == 'G');
      buf[ARRAY_COUNT(buf) - 1 - cursor] =  uppercase ? 'E' : 'e';
      cursor += 1;

      assert(digits_string.size > 0);
      cursor += digits_string.size + 1;
      buf[ARRAY_COUNT(buf) - cursor] = digits_string.data[0];
      buf[ARRAY_COUNT(buf) - cursor + 1] = '.';
      memcpy(&buf[ARRAY_COUNT(buf) - cursor + 2], digits_string.data + 1, digits_string.size - 1);

      String string = string_create(buf + ARRAY_COUNT(buf) - cursor, cursor);

      _format_print_number_with_padding(sb, flags, string, prefix, suffix, field_width);
    }else{
      // TODO: Fuck, this would require proper segmenting. Ouch!
      panic("TODO!");
    }
  }
}

static void _format_print_u64(StringBuilder *sb, U64 value, EnumU32(FORMAT_F_xxx) flags, bool uppercase, I64 field_width, I64 precision, I64 base, String prefix){
  if((flags & FORMAT_F_order_of_magnitudes)){
    assert(base == 10 && "This option is only compatible with base 10!");
    String suffix = LIT_STR("");
    F64 f64;
    if(value >= 1000000000000){
      f64 = value / 1000000000000.0;
      suffix = LIT_STR("T");
    }else if(value >= 1000000000){
      f64 = value / 1000000000;
      suffix = LIT_STR("G");
    }else if(value >= 1000000){
      f64 = value / 1000000;
      suffix = LIT_STR("M");
    }else if(value >= 1000){
      f64 = value / 1000;
      suffix = LIT_STR("k");
    }else{
      f64 = value;
    }

    _format_print_f64(sb, f64, flags, field_width, precision, 'f', prefix, suffix);
  }else if((flags & FORMAT_F_order_of_magnitudes_byte)){
    panic("TODO");
  }else{
    U8 buf[256]; // TODO: Just double check that this buffer is large enough for all uses
    I64 cursor = 0;

    if(value == 0){
      buf[sizeof(buf) - 1 - cursor] = '0';
      cursor += 1;
    }else if(base == 10){
      if(!(flags & FORMAT_F_commas)){
        _format_nonzero_integer_decimal(buf + sizeof(buf), &cursor, value);
      }else{
        while(value){
          U64 t = value / 10;
          U64 digit = value - t * 10;
          value = t;

          if((flags & FORMAT_F_commas) && cursor > 0 && (cursor & 3) == 0){
            buf[sizeof(buf) - 1 - cursor] = ',';
            cursor += 1;
          }

          buf[sizeof(buf) - 1 - cursor] = '0' + digit;
          cursor += 1;
        }
      }
    }else if(base == 8){
      while(value){
        U64 digit = (value & 7);
        value >>= 3;

        buf[sizeof(buf) - 1 - cursor] = '0' + digit;
        cursor += 1;
      }
    }else if(base == 16){
      U8 *set = uppercase ?
                (U8*)"0123456789ABCDEF" :
                (U8*)"0123456789abcdef";
      while(value){
        U64 digit = (value & 15);
        value >>= 4;

        buf[sizeof(buf) - 1 - cursor] = set[digit];
        cursor += 1;
      }
    }else{
      panic("Unknown base value");
    }

    // TODO: Handle precision value
    String string = string_create(buf + sizeof(buf) - cursor, cursor);
    _format_print_number_with_padding(sb, flags, string, prefix, LIT_STR(""), field_width);
  }
}

static void sb_vprintf(StringBuilder *sb, const char *_format, va_list arg_list){
  String format = string_from_cstring(_format);

  // NOTES(hanna):
  // - I try to use the same terminology as the man page for printf here.
  // - We don't support "*m$"-style things like the generic printf does, as looking up something in the arg list is annoying, and I don't personally ever use that feature.
  // - Length modifiers are so fucking stupid. The only thing which truly wants varargs in C is printf, yet they don't promote all integer types to the widest integer type available, only to int...

  U8 *format_at = format.data;
  U8 *format_end = format.data + format.size;

  while(format_at < format_end){
#if ARCH_X86_64 // If we have x64 it is safe to assume we have SSE
    {
      U8 *begin_at = format_at;

      // NOTE(hanna): I have tried copying within our loop instead of memcpy-ing at the end, but that turned out to be slower.

      // First we align ourselves so we can safely read 16 bytes at a time (even if it is technically reading data beyond the buffer, i.e. a buffer overrun)
      while(((uintptr_t)format_at & 0xf) && format_at < format_end){
        if(*format_at == '%'){
          goto found_percent;
        }
        format_at += 1;
      }

      // TODO: The clang sanitizer is going to complain about this!

      while(format_at < format_end){
        __m128i bytes = _mm_load_si128((__m128i*)format_at); // Yes, here we load things which might not belong to us. However we clamp our `format_at` cursor so this should be fine.

        __m128i percent = _mm_set1_epi8('%');
        U32 eq = _mm_movemask_epi8(_mm_cmpeq_epi8(bytes, percent));
        if(eq){
          format_at += index_of_low_bit_u32(eq);
          break;
        }else{
          format_at += 16;
        }
      }
      if(format_at > format_end){ // If we overshoot, bounce back
        format_at = format_end;
      }
      found_percent:;

      uintptr_t size = (uintptr_t)format_at - (uintptr_t)begin_at;
      U8 *buf = sb_append_buffer(sb, size);
      if(buf){
        memcpy(buf, begin_at, size);
      }
    }
#else
    {
      U8 *begin_at = format_at;
      while(format_at < format_end){
        if(*format_at == '%'){
          break;
        }
        format_at += 1;
      }

      uintptr_t size = (uintptr_t)format_at - (uintptr_t)begin_at;
      U8 *buf = sb_append_buffer(sb, size);
      if(buf){
        memcpy(buf, begin_at, size);
      }
    }
#endif

    if(format_at >= format_end){
      break;
    }
    // Next up: Parse the conversion specification
    assert(*format_at == '%');
    format_at += 1; // skip %

    U32 flags = 0; /* FORMAT_F_xxx */

    // Parse flag characters
    while(format_at < format_end){
      switch(*format_at){
        case '#':{
          flags |= FORMAT_F_alternative_form;
        }break;

        case '0':{
          flags |= FORMAT_F_zero_pad;
        }break;

        case '-':{
          flags |= FORMAT_F_left_adjust;
        }break;

        case ' ':{
          flags |= FORMAT_F_blank_positives;
        }break;

        case '+':{
          flags |= FORMAT_F_plus;
        }break;

        case '\'':{
          flags |= FORMAT_F_commas;
        }break;

        case '$':{
          if(format_at + 2 <= format_end && format_at[1] == '$'){
            format_at += 1;
            flags |= FORMAT_F_order_of_magnitudes_byte;
          }else{
             flags |= FORMAT_F_order_of_magnitudes;
            }
        }break;

        default: goto done_parsing_flags;
      }
      format_at += 1;
    }
    done_parsing_flags:;

    // Parse field width and precision
    I64 field_width = _format_parse_int(&format_at, format_end, arg_list);
    I64 precision = -1; // Meaning precision omitted
    if(format_at < format_end && format_at[0] == '.'){
      format_at += 1;
      precision = _format_parse_int(&format_at, format_end, arg_list);
    }

    // Parse length modifier
    I64 byte_width = -1;
    if(format_at < format_end){
      switch(format_at[0]){

        case 'h':{
          format_at += 1;
          if(format_at < format_end && format_at[0] == 'h'){
            format_at += 1;
            byte_width = sizeof(char);
          }else{
            byte_width = sizeof(short);
          }
        }break;

        case 'l':{
          format_at += 1;
          if(format_at < format_end && format_at[0] == 'l'){
            format_at += 1;
            byte_width = sizeof(long long);
          }else{
            byte_width = sizeof(long);
          }
        }break;

        case 'L':{
          format_at += 1;
          byte_width = sizeof(long double);
        }break;

        case 'j':{
          format_at += 1;
          byte_width = sizeof(uintmax_t);
        }break;

        case 'z':{
          format_at += 1;
          byte_width = sizeof(size_t);
        }break;

        case 't':{
          format_at += 1;
          byte_width = sizeof(ptrdiff_t);
        }break;

        case 'I':{
          if(format_at + 3 <= format_end){
            if(format_at[1] == '6' && format_at[2] == '4'){
              format_at += 3;
              byte_width = 8;
            }else if(format_at[1] == '3' && format_at[2] == '2'){
              format_at += 3;
              byte_width = 4;
            }
          }
        }break;
      }
    }

    // Parse format specifiers
    if(format_at >= format_end) panic("Format string missing conversion specifier");
    U8 conversion_specifier = format_at[0];
    format_at += 1;
    switch(conversion_specifier){
      case 'd':
      case 'i':
      {
        I64 value = _format_get_signed_arg(byte_width, arg_list);

        bool negative = false;
        if(value < 0){
          negative = true;
          // TODO: Handle -9223372036854775808 correctly
          value = -value;
        }

        String prefix = LIT_STR("");
        if(negative){
          prefix = LIT_STR("-");
        }else if((flags & FORMAT_F_blank_positives)){
          prefix = LIT_STR(" ");
        }else if((flags & FORMAT_F_plus)){
          prefix = LIT_STR("+");
        }

        _format_print_u64(sb, (U64)value, flags, negative, field_width, precision, 10, prefix);
      }break;

      case 'o':
      case 'u':
      case 'x':
      case 'X':
      {
        I64 base;
        bool uppercase = false;
        if(conversion_specifier == 'o'){ base = 8; }
        else if(conversion_specifier == 'u'){ base = 10; }
        else if(conversion_specifier == 'x'){ base = 16; }
        else if(conversion_specifier == 'X'){ base = 16; uppercase = true; }
        else{ panic("invalid code path"); }

        U64 value = _format_get_unsigned_arg(byte_width, arg_list);

        _format_print_u64(sb, (U64)value, flags, uppercase, field_width, precision, base, LIT_STR(""));
      }break;

      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
      {
        F64 value = va_arg(arg_list, F64);
        _format_print_f64(sb, value, flags, field_width, precision, conversion_specifier, LIT_STR(""), LIT_STR(""));
      }break;

      case 'c':{
        // TODO: Print character
        // TODO: Wide char?
        panic("TODO");
      }break;

      case 's':{
        const char *c_string_value = va_arg(arg_list, const char*);

        String value = string_from_cstring(c_string_value);

        if(precision >= 0){
          value.size = MINIMUM(value.size, precision);
        }

        sb_append_string_padded(sb, value, field_width, !!(flags & FORMAT_F_left_adjust), ' ');
      }break;

      case 'S':{
        String value = va_arg(arg_list, String);

        if(precision >= 0){
          value.size = MINIMUM(value.size, precision);
        }

        sb_append_string_padded(sb, value, field_width, !!(flags & FORMAT_F_left_adjust), ' ');
      }break;

      case 'p':{
        void *ptr_value = va_arg(arg_list, void*);
        CT_ASSERT(sizeof(ptr_value) == 8);
        U64 value = (U64)ptr_value;

        _format_print_u64(sb, value, FORMAT_F_zero_pad, false, 16, -1, 16, LIT_STR(""));
      }break;

      case '%':{
        sb_append_string(sb, LIT_STR("%"));
      }break;

      default:{
        panic("Unknown format specifier");
      }break;
    }
  }
}
#else
static void sb_vprintf(StringBuilder *sb, const char *format, va_list arg_list1){
  va_list arg_list2;
  va_copy(arg_list2, arg_list1);

  int required_bytes = stbsp_vsnprintf(NULL, 0, format, arg_list1);

  // HACK: stbsp uses C strings so we must leave room for the zero terminator, but we don't want to keep it
  U8 *data = sb_append_buffer(sb, required_bytes + 1);
  assert(sb->last_chunk->cursor > 0);
  sb->last_chunk->cursor -= 1; // we don't want the zero terminator to take space here
  sb->total_size -= 1; // no zero terminator
  int required_bytes2 = stbsp_vsnprintf((char*)data, required_bytes + 1, format, arg_list2);

  assert(required_bytes == required_bytes2);

  va_end(arg_list2);
}
#endif

static void sb_printf(StringBuilder *sb, const char *format, ...){
  va_list list;
  va_start(list, format);
  sb_vprintf(sb, format, list);
  va_end(list);
}

static void sb_append_sb(StringBuilder *out, StringBuilder *in){
  // TODO: Could be optimized to utilize the current chunk fully before using the next one
  U8 *at = sb_append_buffer(out, in->total_size);
  if(at){
    for(SBChunk *chunk = in->first_chunk; chunk; chunk = chunk->next){
      memcpy(at, chunk->data, chunk->cursor);
      at += chunk->cursor;
    }
  }
}

//
// Error storage
//

typedef struct ErrorRefLocation ErrorRefLocation;
struct ErrorRefLocation{
  String filename;
  String content; // May either be the full file contents of the file which the error is reported in, or be an empty string and then context strings won't be pulled.

  // NOTE(hanna): `line` is the line which `offset_a` is at. `offset_a` and `offset_b` may span multiple lines.
  I64 line;
  I64 offset_a;
  I64 offset_b;
};
static ErrorRefLocation error_ref_location_create(String filename, String content, I64 line, I64 offset_a, I64 offset_b){
  ErrorRefLocation result = {0};
  result.filename = filename;
  result.content = content;
  result.line = line;
  result.offset_a = offset_a;
  result.offset_b = offset_b;
  return result;
}

typedef struct Error Error;
struct Error{
  Error *next;

  // The file, procedure signature and line of the place where the error is pushed in our source code
  String reporting_filename;
  String reporting_proc_signature;
  I32 reporting_line;

  bool has_ref;
  ErrorRefLocation ref;

  String message;
};

typedef struct Errors Errors;
struct Errors{
  Allocator *allocator;
  Error *first, *last;
};
static Errors errors_create(Allocator *allocator){
  Errors result = {0};
  result.allocator = allocator;
  return result;
}

static Error* errors_vpushf(Errors *errors, String reporting_filename, String reporting_proc_signature, I32 reporting_line, const char *format, va_list list){
  Error *result = NULL;
  if(errors){
    result = allocator_push_item_clear(errors->allocator, Error);
    if(errors->first){
      assert(errors->last);
      errors->last->next = result;
      errors->last = result;
    }else{
      assert(!errors->last);
      errors->first = errors->last = result;
    }
    result->reporting_filename = reporting_filename;
    result->reporting_proc_signature = reporting_proc_signature;
    result->reporting_line = reporting_line;
    result->message = allocator_push_vprintf(errors->allocator, format, list);
  }
  return result;
}
static Error* _errors_pushf(Errors *errors, String reporting_filename, String reporting_proc_signature, I32 reporting_line, const char *format, ...){
  Error *result = NULL;
  if(errors){
    va_list list;
    va_start(list, format);
    result = errors_vpushf(errors, reporting_filename, reporting_proc_signature, reporting_line, format, list);
    va_end(list);
  }
  return result;
}
#define errors_pushf(_errors_, ...) _errors_pushf((_errors_), LIT_STR(__FILE__), LIT_STR(__PRETTY_FUNCTION__), __LINE__, __VA_ARGS__)

static Error* errors_ref_vpushf(Errors *errors, String reporting_filename, String reporting_proc_signature, I32 reporting_line, ErrorRefLocation ref, const char *format, va_list list){
  Error *result = NULL;
  if(errors){
    result = errors_vpushf(errors, reporting_filename, reporting_proc_signature, reporting_line, format, list);
    assert(result);

    result->has_ref = true;
    result->ref = ref;
  }
  return result;
}

static Error* _errors_ref_pushf(Errors *errors, String reporting_filename, String reporting_proc_signature, I32 reporting_line, ErrorRefLocation ref, const char *format, ...){
  Error *result = NULL;
  if(errors){
    va_list list;
    va_start(list, format);
    result = errors_ref_vpushf(errors, reporting_filename, reporting_proc_signature, reporting_line, ref, format, list);
    va_end(list);
  }
  return result;
}

#define errors_ref_pushf(_errors_, ...) _errors_ref_pushf((_errors_), LIT_STR(__FILE__), LIT_STR(__PRETTY_FUNCTION__), __LINE__, __VA_ARGS__)

static bool errors_any(Errors *errors){
  return (errors && errors->first != NULL);
}
static String errors_to_string_simple(Errors *errors, Allocator *allocator){
  Allocator *temp = temp_begin();
  StringBuilder sb = sb_create(temp);
  if(errors){
    for(Error *error = errors->first; error; error = error->next){
      sb_printf(&sb, "{%.*s:%d:%.*s: %.*s}", StrFormatArg(error->reporting_filename), error->reporting_line, StrFormatArg(error->reporting_proc_signature), StrFormatArg(error->message));
    }
  }
  String result = sb_to_string(&sb, allocator);
  temp_end(&temp);
  return result;
}

static String errors_with_refs_to_string(Errors *errors, Allocator *allocator, bool vt100){
  // TODO: Let the colors amass!
  Allocator *temp = temp_begin();
  StringBuilder sb = sb_create(temp);
  if(errors){
    for(Error *error = errors->first; error; error = error->next){
      sb_printf(&sb, "\n");
      sb_printf(&sb, "Line of code reporting this error: %.*s:%d(%.*s)\n", StrFormatArg(error->reporting_filename), error->reporting_line, StrFormatArg(error->reporting_proc_signature));
      sb_printf(&sb, "%.*s:%d: %.*s\n", StrFormatArg(error->ref.filename), error->ref.line, StrFormatArg(error->message));
      if(error->ref.content.size){
        String content = error->ref.content;

        assert(error->ref.offset_a >= 0);
        assert(error->ref.offset_b >= 0);

        assert(error->ref.offset_a <= content.size);
        assert(error->ref.offset_b <= content.size);

        Array(String) lines = array_create(String, temp);

        I64 first_line = error->ref.line;

        I64 line_begin0 = string_beginning_of_line(content, error->ref.offset_a);
        I64 line_end0 = string_end_of_line(content, error->ref.offset_a);

        I64 error_line_offset_a = error->ref.offset_a - line_begin0;

        I64 line_begin1 = line_begin0;
        I64 line_end1 = line_end0;

        // Some context lines before
        fiz(4){
          if(!string_prev_line(content, &line_begin0, &line_end0)){
            break;
          }
          first_line -= 1;
          array_insert(&lines, 0, substring(content, line_begin0, line_end0));
        }

        array_push(&lines, substring(content, line_begin1, line_end1));

        I64 last_line = error->ref.line;

        bool multiline_error = false;
        while(line_end1 < error->ref.offset_b){
          if(!string_next_line(content, &line_begin1, &line_end1)){
            panic("Bad offsets");
          }
          multiline_error = true;
          last_line += 1;
          array_push(&lines, substring(content, line_begin1, line_end1));
        }
        I64 error_line_offset_b = error->ref.offset_b - line_begin1;

        I64 last_error_line = last_line;

        // Some context lines after
        fiz(2){
          if(!string_next_line(content, &line_begin1, &line_end1)){
            break;
          }
          last_line += 1;
          array_push(&lines, substring(content, line_begin1, line_end1));
        }

        I64 max_line_number = last_line;
        assert(max_line_number > 0);
        I32 digit_count = 0;
        {
          I64 x = max_line_number;
          while(x > 0){
            x /= 10;
            digit_count += 1;
          }
        }

        assert(first_line + lines.count - 1 == last_line);

        bool in_error = false;

        fjz(lines.count){
          I64 line_number = first_line + j;

          sb_printf(&sb, " %*I64d | %.*s\n", digit_count, line_number, StrFormatArg(lines.e[j]));

          bool now_in_error = (line_number == error->ref.line || in_error);

          I64 cursor = 0;
          if(now_in_error){
            fiz(1 + digit_count + 3){
              sb_printf(&sb, " ");
            }
          }

          bool is_first_error_line = now_in_error && !in_error;
          if(is_first_error_line){
            in_error = true;

            while(cursor < error_line_offset_a){
              sb_printf(&sb, " ");
              cursor += 1;
            }
          }

          if(in_error){
            if(line_number == last_error_line){
              while(cursor < error_line_offset_b){
                sb_printf(&sb, "^");
                cursor += 1;
              }
              in_error = false;
            }else{
              sb_printf(&sb, "^^^ ...");
            }

            sb_printf(&sb, "\n");
          }
        }
      }
    }
  }
  String result = sb_to_string(&sb, allocator);
  temp_end(&temp);
  return result;
}

//
// Stream
//

// TODO: Switch this over to use I64's

typedef struct StreamChunk StreamChunk;
struct StreamChunk{
  StreamChunk *next;
  I64 size;
  U8 data[0];
};

typedef struct Stream Stream;
struct Stream{ // TODO: Consider using a cyclic buffer for this instead.
  Allocator *allocator;

  // TODO: Should totally have a total consumed byte counter

  I64 total_bytes;
  I64 cursor;
  StreamChunk *current_chunk;
  StreamChunk *last_chunk;
};

static Stream stream_create(Allocator *allocator){
  Stream result = {0};
  result.allocator = allocator;
  return result;
}
static void stream_destroy(Stream *stream){
  for(StreamChunk *chunk = stream->current_chunk; chunk;){
    StreamChunk *next = chunk->next;
    allocator_free(chunk, sizeof(StreamChunk) + chunk->size);
    chunk = next;
  }
  clear_item(stream);
}

static void stream_feed(Stream *stream, U8 *data, I64 size){
  StreamChunk *chunk = (StreamChunk*)allocator_alloc_noclear(stream->allocator, sizeof(StreamChunk) + size, 1);
  clear_item(chunk);
  chunk->size = size;
  memcpy(chunk->data, data, size);
  stream->total_bytes += size;

  if(stream->current_chunk){
    stream->last_chunk->next = chunk;
    stream->last_chunk = chunk;
  }else{
    stream->current_chunk = stream->last_chunk = chunk;
  }
}
static void stream_feed_u8 (Stream *stream, U8 value) { stream_feed(stream, (U8*)&value, 1); }
static void stream_feed_u16(Stream *stream, U16 value){ stream_feed(stream, (U8*)&value, 2); }
static void stream_feed_u32(Stream *stream, U32 value){ stream_feed(stream, (U8*)&value, 4); }
static void stream_feed_u64(Stream *stream, U64 value){ stream_feed(stream, (U8*)&value, 8); }

static void stream_consume_chunk(Stream *stream){
  assert(stream->current_chunk);

  assert(stream->total_bytes >= stream->current_chunk->size - stream->cursor);
  stream->total_bytes -= stream->current_chunk->size - stream->cursor;

  StreamChunk *next = stream->current_chunk->next;
  allocator_free(stream->current_chunk, sizeof(StreamChunk) + stream->current_chunk->size);
  stream->current_chunk = next;
  if(!next) stream->last_chunk = NULL;
}

static bool stream_consume(Stream *stream, U8 *out, I64 out_size){
  bool result = false;

  I64 out_cursor = 0;
  I64 chunk_cursor = stream->cursor;
  I64 total_bytes = stream->total_bytes;
  StreamChunk *chunk = stream->current_chunk;
  for(; chunk; (chunk = chunk->next), (chunk_cursor = 0)){
    I64 size = MINIMUM(chunk->size - chunk_cursor, out_size - out_cursor);
    if(out){
      memcpy(out + out_cursor, chunk->data + chunk_cursor, size);
    }
    out_cursor += size;
    chunk_cursor += size;
    total_bytes -= size;

    if(out_cursor >= out_size) break;
  }
  if(out_cursor == out_size){
    result = true;
    for(; stream->current_chunk != chunk;){
      stream_consume_chunk(stream);
    }

    stream->total_bytes = total_bytes;
    stream->cursor = chunk_cursor;
  }

  return result;
}

static bool stream_consume_line_crlf(Stream *stream, Allocator *allocator, String *_line){
  bool result = false;
  String line = {0};

  I64 size = 0;
  I64 cursor = stream->cursor;
  for(StreamChunk *chunk = stream->current_chunk; chunk; (chunk = chunk->next), (cursor = 0)){
    for(; cursor < chunk->size; cursor += 1, size += 1){
      if(cursor + 2 <= chunk->size && chunk->data[cursor + 0] == '\r' && chunk->data[cursor + 1] == '\n'){
        goto found;
      }
    }
  }
  if(0){
    found:;
    result = true;
    assert(size < UINT32_MAX);
    line.data = allocator_push_items_noclear(allocator, U8, size);
    line.size = size;
    bool status = stream_consume(stream, line.data, line.size);
    assert(status);
    status = stream_consume(stream, NULL, 2);
    assert(status);
  }

  *_line = line;
  return result;
}

static void stream_consume_everything_into_buffer(Stream *stream, U8 **_at){
  U8 *at = *_at;
  I64 size = stream->total_bytes;
  bool status = stream_consume(stream, at, size);
  assert(status);
  at += size;
  *_at = at;
}

static String stream_consume_everything_as_string(Stream *stream, Allocator *allocator){
  String result = {0};
  result.size = stream->total_bytes;
  result.data = allocator_push_items_noclear(allocator, U8, result.size);
  stream_consume(stream, result.data, result.size);
  return result;
}

static void stream_expect(Stream *stream, U8 *out, I64 out_size){
  if(!stream_consume(stream, out, out_size)){
    panic("Stream expected %I64i bytes, but there were only %I64i", out_size, stream->total_bytes);
  }
}
static U8  stream_expect_u8 (Stream *stream){ U8 result;  stream_expect(stream, (U8*)&result, sizeof(result)); return result; }
static U16 stream_expect_u16(Stream *stream){ U16 result; stream_expect(stream, (U8*)&result, sizeof(result)); return result; }
static U32 stream_expect_u32(Stream *stream){ U32 result; stream_expect(stream, (U8*)&result, sizeof(result)); return result; }
static U64 stream_expect_u64(Stream *stream){ U64 result; stream_expect(stream, (U8*)&result, sizeof(result)); return result; }

//
// Read file as stream utility
//

typedef struct FileAsStream FileAsStream;
struct FileAsStream{
  Stream stream;
  bool ok;
};

static FileAsStream read_entire_file_as_stream(String path, Allocator *allocator){
  FileAsStream result = {0};

  Allocator *temp = temp_begin();

  // TODO: Optimize if we use this more.
  EntireFile file = read_entire_file(path, temp);
  if(file.ok){
    result.ok = true;
    result.stream = stream_create(allocator);
    stream_feed(&result.stream, file.data, file.size);
  }

  temp_end(&temp);

  return result;
}

//
// Endianess conversion
//

static U64 u64_swap_endianess(U64 value){
  U8 *bytes = (U8*)&value;
  u8_swap(&bytes[0], &bytes[7]);
  u8_swap(&bytes[1], &bytes[6]);
  u8_swap(&bytes[2], &bytes[5]);
  u8_swap(&bytes[3], &bytes[4]);
  return *(U64*)bytes;
}
static U16 u16_swap_endianess(U16 value){
  U8 *bytes = (U8*)&value;
  u8_swap(&bytes[0], &bytes[1]);
  return *(U16*)bytes;
}

//
// Basic job system
//

typedef struct ThreadPool ThreadPool;
typedef struct Worker Worker;
typedef struct Job Job;

typedef void (*JobExecuteProc)(Job *job);

struct Job{
  // Fields you need to set before submitting a job:
  JobExecuteProc execute;

  // Internal fields:
  Job *next;
  // Allowed transitions of these are
  //  JOB_STATUS_queued           --> JOB_STATUS_complete
  //  JOB_STATUS_queued           --> JOB_STATUS_cancelled_queued
  //  JOB_STATUS_cancelled_queued --> JOB_STATUS_complete
#define JOB_STATUS_queued             0 /* initial value */
#define JOB_STATUS_complete           1 /* either the job is done or it has been cancelled */
#define JOB_STATUS_cancelled_queued   2 /* the job has been cancelled but not yet marked as removed from the job queue yet */
  AtomicU32 status;

  // The following bytes are usually user data.
};
CT_ASSERT(sizeof(Job) == 24);

struct Worker{
  ThreadPool *pool;
  U32 number;
  OSThread thread;
};

struct ThreadPool{
  AtomicU32 prof_frame_index; // Set by the outside world to control which frame the profiler is on

  AtomicU32 running;
  Worker *workers[16];
  U32 num_workers;

  AtomicU32 num_jobs_not_complete;
  Job *first_queued;
  Job *last_queued;

  OSMutex mutex;
  Semaphore job_count_semaphore;
};

static void thread_pool_init(ThreadPool *pool, U32 num_workers);
static void thread_pool_destroy(ThreadPool *pool);
static void thread_pool_submit_job(ThreadPool *pool, Job *job);
static bool thread_pool_check_complete(ThreadPool *pool, Job *job);
static void thread_pool_actively_wait_for_job_completion(ThreadPool *pool, Job *job);
static void thread_pool_actively_wait_for_all_completion(ThreadPool *pool);
static void thread_pool_cancel_job(ThreadPool *pool, Job *job);

//~ JOB SYSTEM IMPLEMENTATION

// NOTE: The job count semaphore must be updated outside of this routine.
static void _thread_pool_do_one_job_no_semaphore(ThreadPool *pool){
  os_mutex_lock(&pool->mutex);
  Job *job = pool->first_queued;
  assert(job);

  if(pool->last_queued == job){
    pool->first_queued = pool->last_queued = NULL;
    assert(job->next == NULL);
  }else{
    pool->first_queued = job->next;
    assert(job->next);
  }
  os_mutex_unlock(&pool->mutex);

  if(atomic_read_u32(&job->status) == JOB_STATUS_queued){
    job->execute(job);
  }

  atomic_compare_exchange_u32(&job->status, JOB_STATUS_queued, JOB_STATUS_complete);
  atomic_compare_exchange_u32(&job->status, JOB_STATUS_cancelled_queued, JOB_STATUS_complete);
  assert(atomic_read_u32(&job->status) == JOB_STATUS_complete);
  atomic_sub_u32(&pool->num_jobs_not_complete, 1);
}

static void _thread_pool_worker_thread_entry_point_proc(void *userdata){
  Worker *worker = (Worker*)userdata;
  ThreadPool *pool = worker->pool;

  //m_thread_init();
  U64 frame_index = UINT64_MAX;

  while(true){
    // NOTE: The semaphore serves two purposes:
    // 1. Job count
    // 2. Incremented once for every thread when stopping the thread pool.
    semaphore_wait(&pool->job_count_semaphore);
    if(!atomic_read_u32(&pool->running)){
      break;
    }
#if 0
    U32 current_frame_index = atomic_read_u32(&pool->prof_frame_index);
    if(current_frame_index != frame_index){
      frame_index = current_frame_index;
      assert(frame_index < PROF_FRAME_COUNT);
      prof_set_thread_context(&worker->prof_thread_contexts[frame_index]);
    }
#endif

    _thread_pool_do_one_job_no_semaphore(pool);
  }

//  prof_free_thread_contexts(worker->prof_thread_contexts);
  //m_thread_destroy();
}

static void thread_pool_init(ThreadPool *pool, U32 num_workers){
  os_mutex_init(&pool->mutex);
  semaphore_init(&pool->job_count_semaphore);

  assert(num_workers <= ARRAY_COUNT(pool->workers));
  pool->num_workers = num_workers;
  atomic_store_u32(&pool->running, 1);
  fiz(pool->num_workers){
    Worker *worker = pool->workers[i] = (Worker*)malloc(sizeof(Worker)); // TODO: Do this through the allocator interface
    worker->pool = pool;
    worker->number = i;

    LOCAL_PRINTF(name, 64, "worker %d", (int)i);
    worker->thread = os_start_thread(_thread_pool_worker_thread_entry_point_proc, worker, name);
  }
}
static void thread_pool_destroy(ThreadPool *pool){
#if COMPILER_GCC
  assert(semaphore_get_value(&pool->job_count_semaphore) == 0 && "There are still jobs in the queue!! You can only destroy the thread pool when the queue is empty.");
#endif

  atomic_store_u32(&pool->running, 0);
  fiz(pool->num_workers){ // Wake up all threads so they can die
    semaphore_post(&pool->job_count_semaphore);
  }
  fiz(pool->num_workers){
    Worker *worker = pool->workers[i];
    os_join_thread(worker->thread);
    free(worker);
  }

  os_mutex_destroy(&pool->mutex);
  semaphore_destroy(&pool->job_count_semaphore);
  clear_item(pool);
}

static void thread_pool_submit_job(ThreadPool *pool, Job *job){
  job->next = NULL;
  atomic_store_u32(&job->status, JOB_STATUS_queued);
  atomic_add_u32(&pool->num_jobs_not_complete, 1);

  os_mutex_lock(&pool->mutex);
  if(pool->first_queued){
    assert(pool->last_queued);
    pool->last_queued->next = job;
    pool->last_queued = job;
  }else{
    assert(!pool->last_queued);
    pool->first_queued = pool->last_queued = job;
  }
  os_mutex_unlock(&pool->mutex);

  // NOTE(hanna): Here we post after unlocking the mutex to increase the probability that the mutex is not locked when the other threads wake up.
  //              I have not done any tests to verify that this is actually the case, but it shouldn't hurt to do things this way.
  semaphore_post(&pool->job_count_semaphore);
}

static bool thread_pool_check_complete(ThreadPool *pool, Job *job){
  bool result = false;

  U32 status = atomic_read_u32(&job->status);
  if(status == JOB_STATUS_complete){
    result = true;
  }

  return result;
}

// Waits until the job is complete and if it is not complete it will do other jobs
static void thread_pool_actively_wait_for_job_completion(ThreadPool *pool, Job *job){
  while(true){
    U32 status = atomic_read_u32(&job->status);
    if(status == JOB_STATUS_complete){
      break;
    }

    // TODO: Is 10000 a good number here?
    if(semaphore_timedwait_ns(&pool->job_count_semaphore, 10000)){
      _thread_pool_do_one_job_no_semaphore(pool);
    }
  }
}

static void thread_pool_actively_wait_for_all_completion(ThreadPool *pool){
  while(semaphore_trywait(&pool->job_count_semaphore)){
    _thread_pool_do_one_job_no_semaphore(pool);
  }

  while(atomic_read_u32(&pool->num_jobs_not_complete) > 0){
    // spin!
    // TODO: Do something more reasonable?
#if ARCH_X86_64
    _mm_pause();
#endif
  }
}

// Cancels the job unless it is completed.
static void thread_pool_cancel_job(ThreadPool *pool, Job *job){
  if(atomic_compare_exchange_u32(&job->status, JOB_STATUS_queued, JOB_STATUS_cancelled_queued)){
    // We are done here. The worker threads will remove the job from the queue
  }
}

//
// Color
//

typedef struct Color32 Color32;
struct Color32{
  union{
    U32 u32;
    struct{ U8 r, g, b, a; };
  };
};
static Color32 color32(U32 u32){
  Color32 result = {0};
  result.u32 = u32;
  return result;
}
static Color32 color32_8888(U8 r, U8 g, U8 b, U8 a){
  Color32 result = {0};
  result.r = r;
  result.g = g;
  result.b = b;
  result.a = a;
  return result;
}

static V4 color32_to_v4(Color32 color){
  return vec4( color.r * (1 / 255.0f), color.g * (1 / 255.0f), color.b * (1 / 255.0f), color.a * (1 / 255.0f));
}
static Color32 v4_to_color32(V4 color){
  // TODO: Think about rounding here!!
  return color32_8888(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
}
static Color32 v3_to_color32(V3 color){
  // TODO: Think about rounding here!!
  return color32_8888(color.r * 255, color.g * 255, color.b * 255, 0xff);
}


//==============================================
// BEGIN PLATFORM-SPECIFIC CODE IMPLEMENTATION
//==============================================

#if OS_LINUX

//
// NOTE: Implementation of the API
//

static void *os_alloc_pages_commit(size_t size){
  void *result = NULL;
  result = mmap(NULL, size,
                PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE,
                -1, 0);
  if(result == (void*)-1){
    result = NULL;
  }
  return result;
}
static void *os_alloc_pages_nocommit(size_t size){
  void *result = NULL;
  result = mmap(NULL, size,
                PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_PRIVATE,
                -1, 0);
  if(result == (void*)-1){
    result = NULL;
  }
  return result;
}

static void os_free_pages(void *memory, size_t size){
  munmap(memory, size);
}

static OSFile os_open_file_input(String path){
  OSFile result = {0};
  char _path[4096];
  if(string_to_cstring(path, _path, sizeof(_path))){
    int fd = open(_path, O_RDONLY);
    result.value = (uint64_t)(fd + 1);
  }
  return result;
}
static OSFile os_open_file_output(String path){
  OSFile result = {0};
  char _path[4096];
  if(string_to_cstring(path, _path, sizeof(_path))){
    int fd = open(_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    result.value = (uint64_t)(fd + 1);
  }
  return result;
}
static void os_close_file(OSFile file){
  if(file.value){
    int fd = (int)file.value - 1;
    close(fd);
  }
}
static I64 os_get_file_size(OSFile file){
  I64 result = 0;
  if(file.value){
    int fd = (int)file.value - 1;
    struct stat stat;
    fstat(fd, &stat);
    result = (I64)stat.st_size;
  }
  return result;
}

static U64 _os_timespec_to_unix_us(struct timespec time){
  return (U64)time.tv_sec * 1000000 + (uint64_t)time.tv_nsec / 1000;
}

static U64 os_get_file_modify_time_us(OSFile file){
  U64 result = 0;
  if(file.value){
    int fd = (int)file.value - 1;
    struct stat stat;
    fstat(fd, &stat);
    struct timespec mtime = stat.st_mtim;
    result = _os_timespec_to_unix_us(stat.st_mtim);
  }
  return result;
}
static bool os_read_from_file(OSFile file, I64 offset, U8 *buffer, U32 size){
  bool result = false;
  if(file.value){
    int fd = (int)file.value - 1;
    ssize_t status = pread(fd, buffer, size, (off_t)offset);
    if(status == size){
      result = true;
    }
  }
  return result;
}
static void os_write_to_file(OSFile file, I64 offset, U8 *buffer, U32 size, bool *_error){
  bool error = false;
  if(_error){
    error = *_error;
  }

  if(error){
    // Error has already occured!
  }else if(!file.value){
    error = true;
  }else{
    int fd = (int)file.value - 1;
    ssize_t bytes_written = pwrite(fd, buffer, size, (off_t)offset);
    if((bytes_written == -1 || bytes_written < size)){
      error = true;
    }
  }

  if(_error){
    *_error = error;
  }
}

static OSMappedFile os_begin_memory_map_file_readonly(OSFile file){
  OSMappedFile result = {0};

  if(file.value){
    int fd = (int)file.value - 1;

    result.data_size = os_get_file_size(file);
    result.data = mmap(NULL, result.data_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if((uint64_t)result.data == (uint64_t)-1){
      result = (OSMappedFile){0};
    }
  }

  return result;
}
static void os_end_memory_map_file(OSFile file, OSMappedFile mapped_file){
  if(mapped_file.data){
    munmap(mapped_file.data, mapped_file.data_size);
  }
}


static OSDir os_read_directory_entries(String path, Allocator *allocator){
  Allocator *temp = temp_begin();

  OSDir result = {0};

  DIR *dir = opendir(allocator_push_cstring(temp, path));
  if(dir){
    Array(String) filenames = array_create(String, allocator);

    errno = 0;
    for(struct dirent *entry; (entry = readdir(dir));){
      String filename = string_from_cstring(entry->d_name);
      if(!string_equals(filename, LIT_STR(".")) && !string_equals(filename, LIT_STR(".."))){
        array_push(&filenames, allocator_push_string(allocator, filename));
      }
    }
    if(!errno){
      result.success = true;
      result.entry_count = filenames.count;
      result.entry_filenames = filenames.e;
    }
    closedir(dir); dir = NULL;
  }

  temp_end(&temp);
  return result;
}

static bool os_create_directory(String path){
  Allocator *temp = temp_begin();

  bool result = true;
  if(mkdir(allocator_push_cstring(temp, path), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0){
    result = false;
  }

  temp_end(&temp);
  return result;
}

static OSPathInfo os_get_path_info(String path){
  Allocator *temp = temp_begin();

  OSPathInfo result = {0};

  struct stat stat_buf;
  if(stat(allocator_push_cstring(temp, path), &stat_buf) == 0){
    if((stat_buf.st_mode & S_IFMT) == S_IFREG){
      result.kind = OS_PATH_KIND_file;
    }else if((stat_buf.st_mode & S_IFMT) == S_IFDIR){
      result.kind = OS_PATH_KIND_directory;
    }else{
      result.kind = OS_PATH_KIND_other;
    }

    result.unix_modify_time_us = _os_timespec_to_unix_us(stat_buf.st_mtim);
  }else if(errno == ENOENT){
    result.kind = OS_PATH_KIND_does_not_exist;
  }

  temp_end(&temp);
  return result;
}


static uint64_t os_get_monotonic_time_us(){
  uint64_t result = 0;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  result = (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
  return result;
}
static uint64_t os_get_unix_time_us(){
  uint64_t result = 0;
  struct timeval tv = {0};
  gettimeofday(&tv, NULL);
  result = (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
  return result;
}
static I64 os_get_unix_time(){
  I64 result = 0;
  result = (I64)time(NULL);
  return result;
}

static String os_get_working_directory(Allocator *allocator){
  String result = {0};
  char buf[PATH_MAX];
  if(getcwd(buf, sizeof(buf))){
    result = allocator_push_string(allocator, string_from_cstring(buf));
  }
  return result;
}

static String os_get_home_directory(Allocator *allocator){
  String result = {0};
  char *home = getenv("HOME");
  if(home){
    result = allocator_push_string(allocator, string_from_cstring(home));
  }
  return result;
}

struct PThreadData{
  void (*entry_point)(void*);
  void *userdata;
};

static void* os_pthread_start_routine(void* _data){
  struct PThreadData *data = (struct PThreadData*)_data;
  data->entry_point(data->userdata);
  free(data);
  return 0;
}

static OSThread os_start_thread(void (*entry_point)(void*), void *userdata, String name){
  OSThread result = {0};

  struct PThreadData *data = (struct PThreadData*)malloc(sizeof(struct PThreadData));
  data->entry_point = entry_point;
  data->userdata = userdata;
  char _name[4096];

  if(!data || !string_to_cstring(name, _name, sizeof(_name))){
    // Error!
  }else{
    pthread_attr_t attr;
    if(pthread_attr_init(&attr) != 0){
      // Error!
    }else{
      pthread_t thread_id = 0;

      if(pthread_create(&thread_id, &attr, os_pthread_start_routine, data)){
        // Error!
      }else{
        extern int pthread_setname_np(pthread_t thread, const char *name); // Cannot get this declaration to show up in pthread.h so I put it here instead.
        pthread_setname_np(thread_id, _name);
        result.value = thread_id;
      }

      pthread_attr_destroy(&attr);
    }
  }

  if(result.value == 0){
    free(data);
  }

  return result;
}

static void os_join_thread(OSThread thread){
  pthread_t thread_id = thread.value;
  pthread_join(thread_id, NULL);
}

static OSThread os_get_handle_to_current_thread(){
  OSThread result = {0};
  result.value = pthread_self();
  return result;
}

static void os_sleep_us(uint64_t duration){
  struct timespec sleep_time = (struct timespec){
    .tv_sec = (I64)(duration / LIT_U64(1000000)),
    .tv_nsec = (I64)(1000 * (duration % LIT_U64(1000000)))
  };
  while(nanosleep(&sleep_time, &sleep_time) == -1 && errno == EINTR);
}

static U64 os_get_entropy_u64(){
  U64 result;
  ssize_t getrandom_return = getrandom(&result, sizeof(result), 0);
  int getrandom_errno = errno;
  if(getrandom_return < 0){
    panic("getrandom failed: %s", strerror(getrandom_errno));
  }else if(getrandom_return != sizeof(result)){
    panic("getrandom did not return as many random bytes as we asked for");
  }
  return result;
}

#elif OS_WINDOWS

static void* os_alloc_pages_commit(size_t size){
  void *result = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
  return result;
}
static void* os_alloc_pages_nocommit(size_t size){
  void *result = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
  return result;
}
static void os_free_pages(void *memory, size_t size){
  (void)size;
  VirtualFree(memory, 0, MEM_RELEASE);
}

static wchar_t* win32_path_to_wide(Allocator *allocator, String path){
  Array(U16) result = array_create(U16, allocator);

  // NOTE(hanna): First we prepend "\\?\" in order to allow for long paths
  // TODO: Should this be enabled or not??
  /*
  array_push(&result, '\\');
  array_push(&result, '\\');
  array_push(&result, '?');
  array_push(&result, '\\');
  */

  string_utf8_to_utf16_array(&result, path);

  array_push(&result, '\0');

  return (wchar_t*)result.e;
}

static String win32_wide_to_path(Allocator *allocator, wchar_t *wide){
  Array(U8) result = array_create(U8, allocator);
  string_utf16_to_utf8_array(&result, string_utf16_from_cstring(wide));
  return array_u8_as_string(result);
}

static HANDLE win32_get_file_handle(OSFile file){
  if(!file.value){
    panic("Recieved invalid file handle");
  }
  return (HANDLE)(file.value - 1);
}
static HANDLE win32_get_thread_handle(OSThread thread){
  if(!thread.value){
    panic("Received invalid thread handle!");
  }
  return (HANDLE)thread.value;
}

// Unix time in microseconds since the epoch
static U64 win32_filetime_to_unix_us(FILETIME filetime){
  ULARGE_INTEGER large;
  large.LowPart = filetime.dwLowDateTime;
  large.HighPart = filetime.dwHighDateTime;

  U64 modify_time = large.QuadPart;

  // NOTE(hanna): For the conversion formula here, see https://learn.microsoft.com/en-us/windows/win32/sysinfo/converting-a-time-t-value-to-a-file-time
  // and solve for microseconds. TODO: Verify correctness here!!
  return (modify_time / LIT_U64(10) - LIT_U64(11644473600000000));
}

static OSFile os_open_file_input(String path){
  OSFile result = {0};

  Allocator *temp = temp_begin();

  wchar_t *wide_path = win32_path_to_wide(temp, path);
  HANDLE handle = CreateFileW(
    wide_path,
    GENERIC_READ,
    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL);
  if(handle != INVALID_HANDLE_VALUE){
    result.value = (U64)handle + 1;
  }

  temp_end(&temp);
  return result;
}
static OSFile os_open_file_output(String path){
  OSFile result = {0};
  Allocator *temp = temp_begin();

  wchar_t *wide_path = win32_path_to_wide(temp, path);

  HANDLE handle = CreateFileW(
    wide_path,
    GENERIC_WRITE,
    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    NULL);
  if(handle != INVALID_HANDLE_VALUE){
    result.value = (U64)handle + 1;
  }

  temp_end(&temp);
  return result;
}
static void os_close_file(OSFile file){
  if(file.value){
    HANDLE handle = win32_get_file_handle(file);
    CloseHandle(handle);
  }
}
static I64 os_get_file_size(OSFile file){
  I64 result = 0;
  if(file.value){
    HANDLE handle = win32_get_file_handle(file);
    LARGE_INTEGER file_size;
    if(GetFileSizeEx(handle, &file_size)){
      result = file_size.QuadPart;
    }
  }
  return result;
}
static U64 os_get_file_modify_time_us(OSFile file){
  U64 result = 0;
  if(file.value){
    HANDLE handle = win32_get_file_handle(file);
    FILETIME modify_filetime;
    if(GetFileTime(handle, NULL, NULL, &modify_filetime)){
      result = win32_filetime_to_unix_us(modify_filetime);
    }
  }
  return result;
}
static bool os_read_from_file(OSFile file, I64 offset, U8 *buffer, U32 size){
  bool result = false;
  if(file.value){
    HANDLE handle = win32_get_file_handle(file);
    DWORD bytes_read;
    OVERLAPPED overlapped = {0};
    overlapped.Offset = (U32)(offset & LIT_I64(0xFFFFFFFF));
    overlapped.OffsetHigh = (U32)((offset >> 32) & LIT_I64(0x7FFFFFFFF));
    if(ReadFile(handle, buffer, size, &bytes_read, &overlapped) && bytes_read == size){
      result = true;
    }
  }
  return result;
}

static void os_write_to_file(OSFile file, I64 offset, U8 *buffer, U32 size, bool *_error){
  bool error = false;
  if(_error){ error = *_error; }

  if(!file.value){
    error = true;
  }else if(error){
    // An error has already occured, do nothing!
  }else{
    HANDLE handle = win32_get_file_handle(file);
    OVERLAPPED overlapped = {0};
    overlapped.Offset = (U32)(offset & LIT_I64(0xFFFFFFFF));
    overlapped.OffsetHigh = (U32)((offset >> 32) & LIT_I64(0x7FFFFFFFF));
    DWORD bytes_written;
    if(WriteFile(handle, buffer, size, &bytes_written, &overlapped) && bytes_written == size){
      // Success!
    }else{
      error = true;
    }
  }
  if(_error){ *_error = error; }
}

/*
static OSMappedFile os_begin_memory_map_file_readonly(OSFile file){

}
static void os_end_memory_map_file(OSFile file, OSMappedFile mapped_file){
}
*/

static OSDir os_read_directory_entries(String path, Allocator *allocator){
  Allocator *temp = temp_begin();

  OSDir result = {0};

  // TODO: If we ever add a push_printf_cstring we could use that
  String query = allocator_push_printf(temp, "%.*s\\*", StrFormatArg(path));
  wchar_t *wide_query = win32_path_to_wide(temp, query);

  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFile(wide_query, &find_data);
  if(find_handle != INVALID_HANDLE_VALUE){
    Array(String) filenames = array_create(String, allocator);

    do{
      wchar_t *wide = find_data.cFileName;
      bool ignore = (wide[0] == '.' && wide[1] == '\0') || (wide[0] == '.' && wide[1] == '.' && wide[2] == '\0');
      if(!ignore){
        String filename = win32_wide_to_path(allocator, wide);
        array_push(&filenames, filename);
      }
    }while(FindNextFile(find_handle, &find_data));

    int error = GetLastError();
    if(error == ERROR_NO_MORE_FILES){
      result.success = true;
      result.entry_count = filenames.count;
      result.entry_filenames = filenames.e;
    }

    FindClose(find_handle);
  }else if(GetLastError() == ERROR_FILE_NOT_FOUND){
    result.success = true;
  }

  temp_end(&temp);

  return result;
}

static OSPathInfo os_get_path_info(String path){
  Allocator *temp = temp_begin();

  OSPathInfo result = {0};

  wchar_t *wide = win32_path_to_wide(temp, path);

  // TODO: Proper check to see if something is a regular file, I don't think this is sufficient. Apparently this can be done through CreateFile, dunno how that works though
  DWORD attributes = GetFileAttributes(wide);
  if(attributes != INVALID_FILE_ATTRIBUTES){
    if(attributes & FILE_ATTRIBUTE_DIRECTORY){
      result.kind = OS_PATH_KIND_directory;
    }else{
      result.kind = OS_PATH_KIND_file;
    }

//#error "TODO: Get file time here!"
  }

  temp_end(&temp);
  return result;
}

static U64 os_get_monotonic_time_us(){
  LARGE_INTEGER time;
  QueryPerformanceCounter(&time); // Apparently never fails since Windows XP
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq); // Apparently never fails since Windows XP

  return (time.QuadPart * LIT_U64(1000000)) / freq.QuadPart;
}
static U64 os_get_unix_time_us(){
  // NOTE(hanna): I really don't know my way around the Windows API, so this is the solution I have come up with.
  FILETIME filetime;
  GetSystemTimeAsFileTime(&filetime);

  return win32_filetime_to_unix_us(filetime);
}
static I64 os_get_unix_time(){
  return (I64)(os_get_unix_time_us() / LIT_U64(1000000));
}

/*
static String os_get_working_directory(Allocator *allocator){
}
static String os_get_home_directory(Allocator *allocator){
}
*/

typedef struct Win32ThreadData Win32ThreadData;
struct Win32ThreadData{
  void (*entry_point)(void*);
  void *userdata;
};

static DWORD win32_thread_entry_point_proc(LPVOID parameter){
  Win32ThreadData *data = (Win32ThreadData*)parameter;
  data->entry_point(data->userdata);
  free(data); data = NULL;
  return 0;
}

// TODO: We should allow passiong stack size to this routine!
// TODO: Handle name here
static OSThread os_start_thread(void (*entry_point)(void*), void *userdata, String name){
  Win32ThreadData *data = (Win32ThreadData*)calloc(1, sizeof(Win32ThreadData));
  data->entry_point = entry_point;
  data->userdata = userdata;

  HANDLE thread_handle = CreateThread(NULL, 0, win32_thread_entry_point_proc, data, 0, NULL);
  OSThread result = {0};
  if(thread_handle == NULL){
    free(data); data = NULL;
  }else{
    result.value = (U64)thread_handle;
  }
  return result;
}
static void os_join_thread(OSThread thread){
  if(thread.value){
    HANDLE handle = win32_get_thread_handle(thread);
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
  }
}
/*
static OSThread os_get_handle_to_current_thread(){
  OSThread result = {};
}
*/

static void os_sleep_us(uint64_t duration){
  // TODO: For now we don't do anything here as Windows doesn't seem very keen on sleeping for less than a millisecond
//#error "TODO: Actually do some sleeping, do our best!"
}

static U64 os_get_entropy_u64(){
  U64 result;
  if(BCryptGenRandom(NULL, (U8*)&result, sizeof(result), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0){
    panic("BCryptGenRandom failed!");
  }
  return result;
}

#else
#error "Unknown OS!"
#endif

//===============================
// END PLATFORM CODE IMPLEMENTATION
//===============================

#endif // HANNA_UTIL_H

//
// NOTE: LICENSE
//

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/
