/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include "typ.h"
#include "run.h"

extern int Dump;
extern struct target Target[];
extern u32 TargetLen;

#ifdef X86_USE_FLOAT

/**
 * given a candidate function and an input, pass input as a parameter
 * and execute 'f'
 */
static float shim_f(const void *f, float a, float b, float c)
{
  float out;
  __asm__ volatile(
    /* pass in parameter(s) */
    "fld  %2;"
    "fstp    (%%esp);"
    "fld  %3;"
    "fstp 0x4(%%esp);"
    "fld  %4;"
    "fstp 0x8(%%esp);"

    /* call function pointer */
    "call *%1;"
    /* store st(0) in out */
    "fst   %0;"
    /*
     * clear entire (8 member) x86 float stack
     * every time, lest it eventually fill up.
     * this took a while to figure out :-/
     */
    "ffree %%st(0);"
    "ffree %%st(1);"
    "ffree %%st(2);"
    "ffree %%st(3);"
    "ffree %%st(4);"
    "ffree %%st(5);"
    "ffree %%st(6);"
    "ffree %%st(7);"
    : "=m"(out)
    : "r"(f), "m"(a), "m"(b), "m"(c));
  return out;
}

/**
 * given a candidate function, test it against all input and return a
 * score -- a distance from the ideal output.
 * a score of 0 indicates a perfect match against the test input
 */
float score(const void *f, int verbose)
{
  float scor = 0.f;
  u32 i;
  if (verbose || Dump >= 2) {
    printf("%-35s %-23s %-23s\n"
           "----------------------------------- "
           "----------------------- "
           "-----------------------\n"
           "%11s %11s %11s %11s %11s %11s %11s\n"
           "----------- ----------- ----------- "
           "----------- ----------- ----------- -----------\n",
           "Input", "Output", "Difference",
           "a", "b", "c", "expected", "actual", "diff", "sum(diff)");
  }
  for (i = 0; i < TargetLen; i++) {
    float diff,   /* difference between target and sc */
          sc;
    sc = shim_f(f, Target[i].in[0], Target[i].in[1], Target[i].in[2]);
    if (!isfinite(sc)) {
      scor = FLT_MAX;
      if (verbose || Dump >= 2)
        fputc('\n', stdout);
      break;
    }
    diff = Target[i].out - sc;
    diff = fabs(diff);
    /* detect overflow */
    if (FLT_MAX - diff < scor) {
      scor = FLT_MAX;
      break;
    }
    scor += diff;
    if (verbose || Dump >= 2)
      printf("%11g %11g %11g %11g %11g %11g %11g\n",
        Target[i].in[0], Target[i].in[1], Target[i].in[2],
        Target[i].out, sc, diff, scor);
  }
  if (verbose || Dump >= 2)
    printf("score=%g\n", scor);
  return scor;
}

#else /* integer */

static inline u32 shim_i(const void *, u32, u32, u32);

/**
 * given a candidate function, test it against all input and return a
 * score -- a distance from the ideal output.
 * a score of 0 indicates a perfect match against the test input
 */
u32 score(const void *f, int verbose)
{
  volatile u32 scor = 0, i;
  if (verbose || Dump >= 2) {
    printf("%-35s %-23s %-23s\n"
           "----------------------------------- "
           "----------------------- "
           "-----------------------\n"
           "%11s %11s %11s %11s %11s %11s %11s\n"
           "----------- ----------- ----------- "
           "----------- ----------- ----------- -----------\n",
           "Input", "Output", "Difference",
           "a", "b", "c", "expected", "actual", "diff", "sum(diff)");
  }
  for (i = 0; i < TargetLen; i++) {
    volatile u32 sc   = shim_i(f, Target[i].in[0], Target[i].in[1], Target[i].in[2]);
#if 0
    /*
     * bitwise distance, useful for, well, bitwise functions
     */
    u32 diff = popcnt(sc ^ Target[i].out);
#else
    /*
     * arithmetic distance, useful for arithmetic functions
     */
    u32 diff = (u32)abs((s32)Target[i].out - (s32)sc);
    if (0xFFFFFFFFU - diff < scor) {
      scor = 0xFFFFFFFFU;
      break;
    }
#endif
    scor += diff;
    if (verbose || Dump >= 2)
      printf(" 0x%08" PRIx32 "  0x%08" PRIx32 "  0x%08" PRIx32
            "  0x%08" PRIx32 "  0x%08" PRIx32 " %11" PRIu32 " %11" PRIu32 "\n",
        Target[i].in[0], Target[i].in[1], Target[i].in[2],
        Target[i].out, sc, diff, scor);
  }
  if (verbose || Dump >= 2)
    printf("score=%u\n", scor);
  return scor;
}

/**
 * execute f(in); ensure no collateral damage
 */
static inline u32 shim_i(const void *f, u32 x, u32 y, u32 z)
{
  volatile u32 out;
  __asm__ volatile(
    /*
     * zero all registers to ensure candidate
     * function doesn't have access to anything
     * but zeroes
     *
     * NOTE: Apple doesn't like when i clobber
     * the ebx register, so we'll just push and
     * pop it manually...
     */
#ifdef __x86_64__
    "pushq %%rbx;"
    "pushq %%rcx;"
    "pushq %%rdx;"
    "pushq %%rdi;"
    "pushq %%rsi;"
#else
    "push %%ebx;"
    "push %%ecx;"
    "push %%edx;"
    "push %%edi;"
    "push %%esi;"
#endif
    /* pass in parameters */
    "movl %4,0x8(%%esp);"
    "movl %3,0x4(%%esp);"
    "movl %2,   (%%esp);"
    /* zero regs */
    "xor  %%eax, %%eax;"
    "xor  %%ebx, %%ebx;"
    "xor  %%ecx, %%ecx;"
    "xor  %%edx, %%edx;"
    "xor  %%edi, %%edi;"
    "xor  %%esi, %%esi;"
    /* call function pointer */
    "call *%1;"
#ifdef __x86_64__
    "popq %%rsi;"
    "popq %%rdi;"
    "popq %%rdx;"
    "popq %%rcx;"
    "popq %%rbx;"
#else
    "pop  %%esi;"
    "pop  %%edi;"
    "pop  %%edx;"
    "pop  %%ecx;"
    "pop  %%ebx;"
#endif
    : "=a"(out)
    : "m"(f), "c"(x), "d"(y), "D"(z));
  return out;
}

#endif

u32 popcnt(u32 n)
{
  /*
   * store popcnts for all octet values
   */
  static const u8 Cnt[] = 
  {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
  };
#if 0
  /*
   * this is much prettier but requires
   * optimization to be fast
   */
  return Cnt[(u8) n       ]
       + Cnt[(u8)(n >>  8)]
       + Cnt[(u8)(n >> 16)]
       + Cnt[(u8)(n >> 24)];
#else
  /*
   * this is a faster (sans optimizations),
   * uglier equivalent
   */
  register u32 m = n;
  register u32 cnt;
  cnt  = Cnt[(u8)m]; m >>= 8;
  cnt += Cnt[(u8)m]; m >>= 8;
  cnt += Cnt[(u8)m]; m >>= 8;
  cnt += Cnt[(u8)m];
  return cnt;
#endif
}


