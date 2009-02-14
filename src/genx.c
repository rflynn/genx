/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */
/*
 * FIXME: still, very occasionally (once every billion genotypes or so)
 *        produces code that crashes(!)
 */

#define _XOPEN_SOURCE 500
#include <stdlib.h>

#include <sys/wait.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include "typ.h"
#include "rnd.h"
#include "x86.h"
#include "gen.h"

#define GEN_DEADEND   1000      /* start over after this many generations of no progress */
#define POP_KEEP      16        /* keep this many from generation to generation */
#define MINIMIZE_LEN  1         /* do we care about finding the shortest solution, or just any match? */

extern const struct x86 X86[X86_COUNT];

/******************* BEGIN INPUT PART *****************************/

/*
 * the function for which we try to find an equivalence.
 * if you don't *know* what the function is, then just populate
 * Target instead
 */
static int magic(int a)
{
  return (int)sqrt(a);
}

/*
 * the inputs we run against magic() to produce our outputs;
 * when we test candidates we give them the same output and
 * sum the difference from magic(input)
 */
/* NOTE: lib-ize so we can include this in a separate program */
static const int Input[] = {
  0,
  7,
  21,
  49,
  105,
  217,
  441,
  889,
  1785,
  3577,
  7161,
  14329,
  28665,
  57337,
  114681,
  229369,
  458745,
  917497,
  1835001,
  3670009,
  7340025
};

static struct target {
  int in,
      out;
} Target[256] = {
 { 0, 0 }
};
static u32 TargetLen = 0;

/********************* END INPUT PART *****************************/

static void calc_target(const int in[], unsigned cnt)
{
  unsigned i;
  TargetLen = (int)cnt;
  for (i = 0; i < TargetLen; i++) {
    Target[i].in = in[i];
    Target[i].out = magic(in[i]);
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
static inline int asm_func_shim(func f, int in)
{
  int out;
  __asm__(
    /* save and zero regs to prevent cheating by called function */
    /* note: eax contains param 'in' */
    "push %%ebx\n"
    "push %%ecx\n"
    "push %%edx\n"
    "xor  %%ebx, %%ebx\n"
    "xor  %%ecx, %%ecx\n"
    "xor  %%edx, %%edx\n"
    /* pass in first parameter */
    "movl %1, (%%esp)\n"  
    /* call function pointer */
    "call *%2\n"          
    /* restore regs */
    "pop  %%edx\n"
    "pop  %%ecx\n"
    "pop  %%ebx\n"
    : "=a" (out)
    : "r" (in), "m" (f));
  return out;
} 

/**
 * given a candidate function, test it against all input and return a
 * score -- a distance from the ideal output.
 * a score of 0 indicates a perfect match against the test input
 */
u32 score(func f, int verbose)
{
  u32 score = 0;
  u32 i;
  if (verbose)
    printf("%11s %11s %11s %11s %11s\n"
           "----------- ----------- ----------- ----------- -----------\n",
           "in", "target", "actual", "diff", "diffsum");
  for (i = 0; i < TargetLen; i++) {
    int diff,   /* difference between target and sc */
        sc = asm_func_shim(f, i);
    diff = Target[i].out - sc;
    diff = abs(diff);
    /* detect overflow */
    if ((u32)(INT_MAX - diff) < score) {
      score = (u32)0xFFFFFFFF;
      break;
    }
    score += diff;
    if (verbose)
      printf("%11d %11d %11d %11d %11" PRIu32 "\n",
        Target[i].in, Target[i].out, sc, diff, score);
  }
  if (verbose)
    printf("score=%" PRIu32 "\n", score);
  return score;
}

int Dump = 0; /* verbosity level */

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
  CurrBest.score = ~0;
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
        printf("score=%" PRIu32 "\n", Pop.indiv[0].score);
        /* show detailed score from best candidate */
        (void)gen_compile(&Pop.indiv[0].geno, x86);
        (void)score((func)x86, 1);
        if (progress) {
          memcpy(&CurrBest, Pop.indiv, sizeof CurrBest);
          gen_since_best = 0;
        }
      }
    }
    if (GEN_DEADEND == gen_since_best && !progress) {
      gen_dump(&Pop.indiv[0].geno, stdout);
      (void)gen_compile(&Pop.indiv[0].geno, x86);
      (void)score((func)x86, 1);
      printf("No progress for %u generations, trying something new...\n", GEN_DEADEND);
      pop_gen(&Pop, 0, Cross_Rate, Mutate_Rate); /* start over! */
      gen_since_best = 0;
    } else {
      pop_gen(&Pop, POP_KEEP, Cross_Rate, Mutate_Rate); /* retain best from previous */
    }
    generation++;
    gen_since_best++;
  } while (
    0 != CurrBest.score /* perfect match */
    /* favor shorter solutions even after perfect match found */
    || FUNC_PREFIX_LEN + FUNC_SUFFIX_LEN + 1 != Pop.indiv[0].geno.len
  );
  printf("done.\n");
  (void)gen_compile(&Pop.indiv[0].geno, x86);
  (void)score((func)x86, 1);
  return 0;
}

