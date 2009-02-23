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
#define POP_KEEP      8         /* keep this many from generation to generation */
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

  return x * 0.5f;

#if 0
  /* infamous Quake 3 sqrt */
  float xhalf = 0.5f * x;
  int i = *(int*)&x;              /* get bits for floating value */
  i = 0x5f3759df - (i>>1);        /* gives initial guess y0 */
  x = *(float*)&i;                /* convert bits back to float */
  x = x * (1.5f - xhalf * x * x); /* Newton step, repeating increases accuracy */
  return x;
#endif

}

/*
 * the inputs we run against magic() to produce our outputs;
 * when we test candidates we give them the same output and
 * sum the difference from magic(input)
 */
/* NOTE: lib-ize so we can include this in a separate program */
static const float Input[] = {
  0.f,
  1.f,
  2.f,
  3.f,
  4.f,
  1e1f,
  1e2f,
  1e3f,
  1e4f,
  1e5f,
  1e6f,
  1e7f,
  1e10f
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
  float out,
        wtf;
  asm volatile(
    /* save and zero regs to prevent cheating by called function */
    /* note: eax contains param 'in' */
    "push %%eax;"
    "push %%ebx;"
    "push %%ecx;"
    "push %%edx;"
    "xor  %%eax, %%eax;"
    "xor  %%ebx, %%ebx;"
    "xor  %%ecx, %%ecx;"
    "xor  %%edx, %%edx;"
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
    "fstp  %3;"
    "fstp  %3;"
    "fstp  %3;"
    "fstp  %3;"
    "fstp  %3;"
    "fstp  %3;"
    "fstp  %3;"
    "fstp  %3;"
    /* restore regs */
    "pop  %%edx;"
    "pop  %%ecx;"
    "pop  %%ebx;"
    "pop  %%eax;"
    : "=m"(out)
    : "m"(in), "m"(f), "m"(wtf));
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
  //assert(scor > 0.f);
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
        gen_dump(&Pop.indiv[0].geno, stdout);
        printf("->score=%g\n", Pop.indiv[0].score);
        /* show detailed score from best candidate */
        (void)gen_compile(&Pop.indiv[0].geno, x86);
        (void)score((func)x86, 1);
        if (progress) {
          assert(Pop.indiv[0].score < CurrBest.score);
          memcpy(&CurrBest, Pop.indiv, sizeof CurrBest);
          gen_since_best = 0;
        }
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
  } while (0.f != CurrBest.score); /* perfect match */
  printf("done.\n");
  (void)gen_compile(&CurrBest.geno, x86);
  (void)score((func)x86, 1);
  return 0;
}

