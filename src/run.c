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
#ifdef linux
# include <sys/mman.h> /* mmap */
# include <sys/types.h>
# include <fcntl.h>
#endif
#include "typ.h"
#include "x86.h"
#include "run.h"

extern int Dump;

#if 0
static float score_f(const void *f, int verbose);
static u32   score_i(const void *f, int verbose);
#endif

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
static float score_f(const void *f, int verbose)
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

static u32 shim_i(const void *, u32, u32, u32) NOINLINE;
static u32 popcnt(u32 n);

static u8 *x86;
#define X86_BUFLEN 4096

void run_init(void)
{
#ifdef linux
  x86 = mmap(0, X86_BUFLEN, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  if (MAP_FAILED == x86) {
    perror("mmap");
    abort();
  }
#else
  x86 = malloc(X86_BUFLEN);
  assert(NULL != x86);
#endif
  printf("x86=%p\n", (void *)x86);
}

/**
 * given a candidate function, test it against all input and return a
 * score -- a distance from the ideal output.
 * a score of 0 indicates a perfect match against the test input
 */
void score(genoscore *g, const genx_iface *iface, int verbose)
{
  volatile u32 scor = 0, i;
  u32 targetsum = 0,
      testcnt,
      x86len = gen_compile(&g->geno, x86, X86_BUFLEN);
  if (Dump > 0)
    x86_dump(x86, x86len, stdout);
  if (Dump > 1)
    (void)gen_dump(&g->geno, stdout);
  //assert(score(x86, 1) == GENOSCORE_SCORE(g));
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
  testcnt = iface->test.i.data.len;
  for (i = 0; i < testcnt; i++) {
    volatile u32 sc   = shim_i(x86, iface->test.i.data.list[i].in[0],
                                    iface->test.i.data.list[i].in[1],
                                    iface->test.i.data.list[i].in[2]);
    u32 diff;
    targetsum += iface->test.i.data.list[i].out;
    if (SCORE_BIT == iface->test.i.score) {
      /*
       * bitwise distance, useful for, well, bitwise functions
       */
      diff = popcnt(sc ^ iface->test.i.data.list[i].out);
    } else {
      /*
       * arithmetic distance, useful for arithmetic functions
       */
      diff = (u32)abs((s32)iface->test.i.data.list[i].out - (s32)sc);
    }
    if (0xFFFFFFFFU - diff < scor) {
      scor = 0xFFFFFFFFU;
      break;
    }
    scor += diff;
    if (verbose || Dump >= 2)
      printf(" 0x%08" PRIx32 "  0x%08" PRIx32 "  0x%08" PRIx32
            "  0x%08" PRIx32 "  0x%08" PRIx32 " %11" PRIu32 " %11" PRIu32 "\n",
        iface->test.i.data.list[i].in[0],
        iface->test.i.data.list[i].in[1],
        iface->test.i.data.list[i].in[2],
        iface->test.i.data.list[i].out,
        sc, diff, scor);
  }
  if (verbose || Dump >= 2) {
    printf("score=%" PRIu32 "/%" PRIu32 " (%.7f%%)\n",
      scor, targetsum,
      100. - (((double)scor / (double)targetsum) * 100.));
  }
  g->score.i = scor;
}

/**
 * execute f(in); ensure no collateral damage
 */
static u32 shim_i(const void *f, u32 x, u32 y, u32 z)
{
  volatile u32 out;
  __asm__ volatile(
    /*
     * zero all registers to ensure candidate
     * function doesn't have access to anything
     * but zeroes
     */
#ifdef __x86_64__
    "pushq %%rdx;"
    "pushq %%rdi;"
    "pushq %%rsi;"
#else
    "push %%edx;"
    "push %%edi;"
    "push %%esi;"
#endif
    /* pass in parameters */
    /* zero regs */
    "xor  %%edx, %%edx;"
    "xor  %%edi, %%edi;"
    "xor  %%esi, %%esi;"
    /* call function pointer */
    "call *%1;"
#ifdef __x86_64__
    "popq %%rsi;"
    "popq %%rdi;"
    "popq %%rdx;"
#else
    "pop  %%esi;"
    "pop  %%edi;"
    "pop  %%edx;"
#endif
    : "=a"(out)
    : "m"(f), "a"(x), "b"(y), "c"(z));
  return out;
}

#endif

static u32 popcnt(u32 n)
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


