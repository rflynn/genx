/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */

#define _XOPEN_SOURCE 500
#include <stdlib.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <math.h>
#include "typ.h"
#include "rnd.h"
#include "x86.h"
#include "gen.h"

#define GEN_DEADEND   1000      /* start over after this many generations of no progress */
#define POP_KEEP      3         /* keep this many from generation to generation */
#define MINIMIZE_LEN  1         /* do we care about finding the shortest solution, or just any match? */

extern const struct x86 X86[X86_COUNT];

/******************* BEGIN INPUT PART *****************************/

/*
 * the function for which we try to find an equivalence.
 * if you don't *know* what the function is, then just populate
 * Target instead
 */
static float magic(float x)
{

  //return x * 0.5f;
  //return 1.5;
  //return sin(x);   // FSIN
  //return M_PI;     // FLDPI
  //return floor(x); // FLD
  //return ceil(x);
  //return fmod(x, 5.f);
  //return x - 1;
  //return x * 100.f;

  /* infamous Quake 3 inv sqrt */
  float magic = 0x5f3759df;
  float xhalf = 0.5f * x;
  int i = *(int*)&x;              /* get bits for floating value */
  i = magic - (i>>1);             /* gives initial guess y0 */
  x = *(float*)&i;                /* convert bits back to float */
  x = x * (1.5f - xhalf * x * x); /* Newton step, repeating increases accuracy */
  return x;

/*
c7 45 f0 00 00 00 00    movl   $0x0,-0x10(%ebp)

here is the what inv_sqrt compiles to with -O0:

080483a4 <inv_sqrt>:
 80483a4:       55                      push   %ebp
 80483a5:       89 e5                   mov    %esp,%ebp
 80483a7:       83 ec 1c                sub    $0x1c,%esp
 80483aa:       b8 b4 6e be 4e          mov    $0x4ebe6eb4,%eax
 80483af:       89 45 f8                mov    %eax,-0x8(%ebp)
 80483b2:       d9 45 08                flds   0x8(%ebp)
 80483b5:       d9 05 40 85 04 08       flds   0x8048540
 80483bb:       de c9                   fmulp  %st,%st(1)
 80483bd:       d9 5d fc                fstps  -0x4(%ebp)
 80483c0:       8d 45 08                lea    0x8(%ebp),%eax
 80483c3:       8b 00                   mov    (%eax),%eax
 80483c5:       89 45 f4                mov    %eax,-0xc(%ebp)
 80483c8:       8b 45 f4                mov    -0xc(%ebp),%eax
 80483cb:       d1 f8                   sar    %eax
 80483cd:       50                      push   %eax
 80483ce:       db 04 24                fildl  (%esp)
 80483d1:       8d 64 24 04             lea    0x4(%esp),%esp
 80483d5:       d9 45 f8                flds   -0x8(%ebp)
 80483d8:       de e1                   fsubp  %st,%st(1)
 80483da:       d9 7d ee                fnstcw -0x12(%ebp)
 80483dd:       0f b7 45 ee             movzwl -0x12(%ebp),%eax
 80483e1:       b4 0c                   mov    $0xc,%ah
 80483e3:       66 89 45 ec             mov    %ax,-0x14(%ebp)
 80483e7:       d9 6d ec                fldcw  -0x14(%ebp)
 80483ea:       db 5d e8                fistpl -0x18(%ebp)
 80483ed:       d9 6d ee                fldcw  -0x12(%ebp)
 80483f0:       8b 45 e8                mov    -0x18(%ebp),%eax
 80483f3:       89 45 f4                mov    %eax,-0xc(%ebp)
 80483f6:       8d 45 f4                lea    -0xc(%ebp),%eax
 80483f9:       8b 00                   mov    (%eax),%eax
 80483fb:       89 45 08                mov    %eax,0x8(%ebp)
 80483fe:       d9 45 08                flds   0x8(%ebp)
 8048401:       d8 4d fc                fmuls  -0x4(%ebp)
 8048404:       d9 45 08                flds   0x8(%ebp)
 8048407:       de c9                   fmulp  %st,%st(1)
 8048409:       d9 05 44 85 04 08       flds   0x8048544
 804840f:       de e1                   fsubp  %st,%st(1)
 8048411:       d9 45 08                flds   0x8(%ebp)
 8048414:       de c9                   fmulp  %st,%st(1)
 8048416:       d9 5d 08                fstps  0x8(%ebp)
 8048419:       8b 45 08                mov    0x8(%ebp),%eax
 804841c:       89 45 e4                mov    %eax,-0x1c(%ebp)
 804841f:       d9 45 e4                flds   -0x1c(%ebp)
 8048422:       c9                      leave
 8048423:       c3                      ret
*/

}

/*
 * the inputs we run against magic() to produce our outputs;
 * when we test candidates we give them the same output and
 * sum the difference from magic(input)
 */
/* NOTE: lib-ize so we can include this in a separate program */
static const float Input[] = {
  1e-5f,
  0.01f,
  1.f,
  2.f,
  3.f,
  1e1f,
  1e3f,
  1e10f,
};

static struct target {
  float in,
        out;
} Target[256] = {
 { 0, 0 }
};
static u32 TargetLen = 0;

/********************* END INPUT PART *****************************/

static void calc_target(const float in[], unsigned cnt)
{
  unsigned i;
  TargetLen = cnt;
  printf("TargetLen <- %u\n", cnt);
  for (i = 0; i < TargetLen; i++) {
    Target[i].in = in[i];
    Target[i].out = magic(in[i]);
    printf("Target %3u %11g -> %11g\n",
      i, Target[i].in, Target[i].out);
  }
}

static void input_init(void)
{
  calc_target(Input, sizeof Input / sizeof Input[0]);
}

/**
 * given a candidate function and an input, pass input as a parameter
 * and execute 'f'
 */
static float asm_func_shim(func f, float in)
{
  float out;
  asm volatile(
    /* save and zero regs to prevent cheating by called function */
    /* note: eax contains param 'in' */
#if 0
    "push %%eax;"
    "push %%ebx;"
    "push %%ecx;"
    "push %%edx;"
    "xor  %%eax, %%eax;"
    "xor  %%ebx, %%ebx;"
    "xor  %%ecx, %%ecx;"
    "xor  %%edx, %%edx;"
#endif
    /* pass in first parameter */
#if 0
    "movl %1, (%%esp);"
#else
    "fld  %1;"
    "fstp (%%esp);"
#endif
    /* call function pointer */
    "call *%2;"
    "fst   %0;"
    /* aha! this took me a long time to figure out...
     * x86 uses a stack for all flops, which is stateful;
     * the stack is 8 deep, so after every function we
     * blow away any possible contents of said stack
     * so as to ensure we do not carry any data on the stack
     * between calls
     */
    "ffree %%st(0);"
    "ffree %%st(1);"
    "ffree %%st(2);"
    "ffree %%st(3);"
    "ffree %%st(4);"
    "ffree %%st(5);"
    "ffree %%st(6);"
    "ffree %%st(7);"
    /* restore regs */
#if 0
    "pop  %%edx;"
    "pop  %%ecx;"
    "pop  %%ebx;"
    "pop  %%eax;"
#endif
    : "=m"(out)
    : "m"(in), "r"(f));
  return out;
}

int Dump = 0; /* verbosity level */

/**
 * given a candidate function, test it against all input and return a
 * score -- a distance from the ideal output.
 * a score of 0 indicates a perfect match against the test input
 */
float score(func f, int verbose)
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
      printf("%11g %11g ", Target[i].in, Target[i].out);
    sc = asm_func_shim(f, Target[i].in);
    if (verbose || Dump >= 2)
      printf("%11g ", sc);
    if (isnan(sc) || isinf(sc)) {
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

int main(int argc, char *argv[])
{
  static struct pop Pop;
  static u8 x86[1024];
  /* mutation rates */
  const double Mutate_Rate    = 0.2,  /*  */
               Cross_Rate     = 0.7;  /*  */
  genoscore    CurrBest;              /* save best found solution */
  u64          indivs         = 0;    /* total creatures created; status */
  u32          generation     = 0,    /* track time; status */
               gen_since_best = 0;    /* dead end counter */
  /* show sizes of our core types in bytes */
  printf("sizeof Pop.indiv[0]=%u\n", sizeof Pop.indiv[0]);
  printf("sizeof Pop=%u\n", sizeof Pop);
  printf("FLT_EPSILON=%f\n", FLT_EPSILON);
  /* sanity-checks */
  assert(CHROMO_MAX > FUNC_PREFIX_LEN + FUNC_SUFFIX_LEN);
  /* one-time initialization */
  x86_init();
  if (argc > 1) {
    Dump += 'd' == argv[1][1];
    Dump += (2 * ('D' == argv[1][1]));
  }
  /* if we haven't got any data input into us, we assume we'll load
   * data from Input
   */
  if (0 == TargetLen)
    input_init();
  CurrBest.score = FLT_MAX;
  CurrBest.geno.len = 0;
  rnd32_init((u32)time(NULL));
  setvbuf(stdout, (char *)NULL, _IONBF, 0); /* unbuffer stdout */
  memset(&Pop, 0, sizeof Pop);
  pop_gen(&Pop, 0, Cross_Rate, Mutate_Rate); /* seminal generation */
  /* evolve */
  /*
   * TODO: clean this up, pull it out of main
   */
  do {
    int progress;
    pop_score(&Pop);  /* test it and sort best ones */
    indivs += sizeof Pop.indiv / sizeof Pop.indiv[0];
    progress = Pop.indiv[0].score < CurrBest.score;
    if (progress || 0 == generation % 100) {
      /* display generation regularly or on progress */
      time_t t = time(NULL);
      printf("GENERATION %5" PRIu32 " %10" PRIu64 " genotypes @%s",
        generation, indivs, ctime(&t));
      if (progress) {
        /* only display best if it's changed; raises signal/noise ration */
        memcpy(&CurrBest, Pop.indiv, sizeof Pop.indiv[0]);
        gen_dump(&CurrBest.geno, stdout);
        printf("->score=%g\n", Pop.indiv[0].score);
        /* show detailed score from best candidate */
        (void)gen_compile(&CurrBest.geno, x86);
        (void)score((func)x86, 1);
        gen_since_best = 0;
      }
    }
    if (gen_since_best < GEN_DEADEND) {
      pop_gen(&Pop, POP_KEEP, Cross_Rate, Mutate_Rate); /* retain best from previous */
    } else {
      /* we have run GEN_DEADEND generations and haven't made any progress;
       * throw out our top genetic material and start fresh, but still
       * retain CurrBest, against which all new candidates are judged */
      gen_dump(&CurrBest.geno, stdout);
      (void)gen_compile(&CurrBest.geno, x86);
      (void)score((func)x86, 1);
      printf("No progress for %u generations, trying something new...\n", GEN_DEADEND);
      pop_gen(&Pop, 0, Cross_Rate, Mutate_Rate); /* start over! */
      gen_since_best = 0;
    }
    generation++;
    gen_since_best++;
  } while (FLT_EPSILON < CurrBest.score); /* perfect match */
  printf("done.\n");
  (void)gen_compile(&CurrBest.geno, x86);
  (void)score((func)x86, 1);
  return 0;
}

