
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
static float shim_f(const void *f, float in)
{
  float out;
  asm volatile(
    /* pass in first parameter */
    "fld  %1;"
    "fstp (%%esp);"
    /* call function pointer */
    "call *%2;"
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
    : "m"(in), "r"(f));
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
  if (verbose || Dump >= 2)
    printf("%11s %11s %11s %11s %11s\n"
           "----------- ----------- ----------- ----------- -----------\n",
           "in", "target", "actual", "diff", "diffsum");
  for (i = 0; i < TargetLen; i++) {
    float diff,   /* difference between target and sc */
          sc;
    if (verbose || Dump >= 2)
      printf("%11" PRIt " %11" PRIt " ", Target[i].in, Target[i].out);
    sc = shim_f(f, Target[i].in);
    if (verbose || Dump >= 2)
      printf("%11g ", sc);
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
      printf("%11g %11g\n", diff, scor);
  }
  if (verbose || Dump >= 2)
    printf("score=%g\n", scor);
  return scor;
}

#else /* integer */

static u32 shim_i(const void *, u32 in);

/**
 * given a candidate function, test it against all input and return a
 * score -- a distance from the ideal output.
 * a score of 0 indicates a perfect match against the test input
 */
u32 score(const void *f, int verbose)
{
  u32 scor = 0;
  u32 i;
  if (verbose || Dump >= 2)
    printf("%11s %11s %11s %11s %11s\n"
           "----------- ----------- ----------- ----------- -----------\n",
           "in", "target", "actual", "diff", "diffsum");
  for (i = 0; i < TargetLen; i++) {
    u32 sc   = shim_i(f, Target[i].in),
        diff = popcnt(sc ^ Target[i].out);
    scor += diff;
    if (verbose || Dump >= 2)
      printf(" 0x%08" PRIx32 "  0x%08" PRIx32 "  0x%08" PRIx32 " %11" PRIu32 " %11" PRIu32 "\n",
        Target[i].in, Target[i].out, sc, diff, scor);
  }
  if (verbose || Dump >= 2)
    printf("score=%u\n", scor);
  return scor;
}

/**
 * given a candidate function and an input, pass input as a parameter
 * and execute 'f'
 */
static u32 shim_i(const void *f, u32 in)
{
  u32 out;
  asm volatile(
    /* save and zero regs to prevent cheating by called function */
    /* note: eax contains param 'in' */
    "push %%ecx;"
    "push %%edx;"
    "xor  %%ecx, %%ecx;"
    "xor  %%edx, %%edx;"
    /* pass in first parameter */
    "movl %1, (%%esp);"
    "xor  %%eax, %%eax;"
    "xor  %%ebx, %%ebx;"
    /* call function pointer */
    "call *%2;"
    "pop  %%edx;"
    "pop  %%ecx;"
    : "=a"(out)
    : "b"(in), "m"(f));
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


