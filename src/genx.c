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
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <math.h>
#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include "typ.h"
#include "rnd.h"
#include "x86.h"
#include "gen.h"
#include "run.h"

#define GEN_DEADEND   1000      /* start over after this many generations of no progress */
#define POP_KEEP      3         /* keep this many from generation to generation */
#define MINIMIZE_LEN  1         /* do we care about finding the shortest solution, or just any match? */

extern const struct x86 X86[X86_COUNT];

/******************* BEGIN INPUT PART *****************************/

#if 0
/*
 * the function for which we try to find an equivalence.
 * if you don't *know* what the function is, then just populate
 * Target instead
 */
static float magic(float x[])
{

  //return x[0] * 0.5f;
  //return 1.5;
  //return sin(x[0]);   // FSIN
  //return M_PI;     // FLDPI
  //return floor(x[0]); // FLD
  //return ceil(x[0]);
  //return fmod(x[0], 5.f);
  //return x[0] - 1;
  //return x[0] * 100.f;

  /* infamous Quake 3 inv sqrt */
  float magic = 0x5f3759df;
  float xhalf = 0.5f * x[0];
  int i = *(int*)x;                   /* get bits for floating value */
  i = magic - (i>>1);                 /* gives initial guess y0 */
  x[0] = *(float*)&i;                 /* convert bits back to float */
  x[0] = x[0]*(1.5f-xhalf*x[0]*x[0]); /* Newton step, repeating increases accuracy */
  return x[0];
}
#endif

#if 0
static s32 magic(s32 x[])
{
  /*
   * we'll try to find a bitwise solution
   * for calculating 1.f/sqrtf(x)
   * in order to try to beat the quake3
   * InvSqrt hack
   *
   * floating point on x86 is inherently slow
   * due to longer cycles per operation and the
   * requirement of all float loads be from memory
   */
  float  a = *(float *)x;   /* type-pun int->float */
  float  b = 1.f/sqrtf(a);  /* calculate true value */
  s32    c = *(s32 *)&b;    /* type-pun back */
  return c;
}
#endif

#ifdef X86_USE_INT
/* test multi-parameter */
static s32 magic(s32 x[])
{
  //return x[0] + x[1];
  //return x[0] * x[1];
  //return (x[0] * x[1]) + x[2];
  //return 0x55555555 ^ x[0]; // ha! found equivalence very quickly, i'm surprised
  //return x[0] / x[1]; // found pretty quickly

  //return x[0] / (((x[2] * x[1]) / x[0]) + 1); // found, not as quickly

  //return x[0] ^ (x[1] << 16) ^ (x[2] >> 3);

  return (s32)sqrt(x[0]);

  //return 0x55555555 | x[0]; // not found yet
}

#endif

struct target Target[] = {
#ifdef X86_USE_FLOAT
  /* air        co2            temp
   * age        ppmv           var (C) */
  {{   2342.f,  284.7f, 0.f }, -0.98f },
  {{  50610.f,  189.3f, 0.f }, -5.57f },
  {{ 103733.f,  225.9f, 0.f }, -4.50f },
  {{ 151234.f,  197.0f, 0.f }, -6.91f },
  {{ 201324.f,  226.4f, 0.f }, -1.49f },
  {{ 401423.f,  277.1f, 0.f }, -1.09f },
#else
  /* random integer data */
  {{ 0xffffffff, 0, 0 }, 0 },
  {{ 0x7fffffff, 0, 0 }, 0 },
  {{ 0x30305123, 0, 0 }, 0 },
  {{ 0x12345678, 0, 0 }, 0 },
  {{    0xaaaaa, 0, 0 }, 0 },
  {{    0xaaaaa, 0, 0 }, 0 },
  {{    0x55555, 0, 0 }, 0 },
  {{    0x44444, 0, 0 }, 0 },
  {{    0x33333, 0, 0 }, 0 },
  {{    0x11111, 0, 0 }, 0 },
  {{      65535, 0, 0 }, 0 },
  {{        450, 0, 0 }, 0 },
  {{        100, 0, 0 }, 0 },
  {{         65, 0, 0 }, 0 },
  {{         12, 0, 0 }, 0 },
  {{          8, 0, 0 }, 0 },
  {{          5, 0, 0 }, 0 },
  {{          4, 0, 0 }, 0 },
  {{          3, 0, 0 }, 0 },
  {{          2, 0, 0 }, 0 },
  {{          1, 0, 0 }, 0 },
  {{          0, 0, 0 }, 0 }
#endif
};
u32 TargetLen = sizeof Target  / sizeof Target[0];

/********************* END INPUT PART *****************************/

#ifdef X86_USE_INT
static void calc_target(void)
{
  unsigned i;
  printf("TargetLen <- %u\n", TargetLen);
  for (i = 0; i < TargetLen; i++) {
    Target[i].out = magic(Target[i].in);
    printf("Target %3u (%11" PRIt ",%11" PRIt ",%11" PRIt ") -> %11" PRIt "\n",
      i, Target[i].in[0], Target[i].in[1], Target[i].in[2], Target[i].out);
  }
}
#endif

int Dump = 0; /* verbosity level */
static time_t Start;

int main(int argc, char *argv[])
{
  static struct pop Pop;
  static u8 x86[1024];
  /* mutation rates */
  const double Mutate_Rate    = 0.2;  /*  */
  genoscore    CurrBest;              /* save best found solution */
  u64          indivs         = 0;    /* total creatures created; status */
  u32          generation     = 0,    /* track time; status */
               gen_since_best = 0;    /* dead end counter */
  /* show sizes of our core types in bytes */
  printf("sizeof Pop.indiv[0]=%lu\n", (unsigned long)(sizeof Pop.indiv[0]));
  printf("sizeof Pop=%lu\n", (unsigned long)(sizeof Pop));
  printf("FLT_EPSILON=%g\n", FLT_EPSILON);
  /* sanity-checks */
  assert(CHROMO_MAX > GEN_PREFIX_LEN + GEN_SUFFIX_LEN);
  /* one-time initialization */
  x86_init();
  if (argc > 1) {
    Dump += 'd' == argv[1][1];
    Dump += (2 * ('D' == argv[1][1]));
  }
  /*
   * if called we set Target[i].out = magic(Target[i].in)
   * if not called, Target[0..TargetLen-1].out assumed set
   */
#ifdef X86_USE_INT
  calc_target();
#endif
  GENOSCORE_SCORE(&CurrBest) = GENOSCORE_MAX;
  CurrBest.geno.len = 0;
  rnd32_init((u32)time(NULL));
  setvbuf(stdout, (char *)NULL, _IONBF, 0); /* unbuffer stdout */
  memset(&Pop, 0, sizeof Pop);
  nice(+19); /* be as polite to any other programs as possible */
  Start = time(NULL);
  pop_gen(&Pop, 0, Mutate_Rate); /* seminal generation */
  /* evolve */
  /*
   * TODO: clean this up, pull it out of main
   */
  do {
    int progress;
    pop_score(&Pop);  /* test it and sort best ones */
    indivs += sizeof Pop.indiv / sizeof Pop.indiv[0];
    progress = -1 == genoscore_lencmp(&Pop.indiv[0], &CurrBest);
    if (progress || 0 == generation % 100) {
      /* display generation regularly or on progress */
      time_t t = time(NULL);
      printf("GENERATION %5" PRIu32 " %10" PRIu64 " genotypes (%.1f/sec) @%s",
        generation, indivs, (double)indivs / (t - Start), ctime(&t));
      if (progress) {
        /* only display best if it's changed; raises signal/noise ration */
        memcpy(&CurrBest, Pop.indiv, sizeof Pop.indiv[0]);
        gen_dump(&CurrBest.geno, stdout);
        printf("->score=%" PRIt "\n",
          GENOSCORE_SCORE(&Pop.indiv[0]));
        /* show detailed score from best candidate */
        (void)gen_compile(&CurrBest.geno, x86);
        (void)score(x86, 1);
        gen_since_best = 0;
      }
    }
    if (gen_since_best < GEN_DEADEND) {
      pop_gen(&Pop, POP_KEEP, Mutate_Rate); /* retain best from previous */
    } else {
      /* we have run GEN_DEADEND generations and haven't made any progress;
       * throw out our top genetic material and start fresh, but still
       * retain CurrBest, against which all new candidates are judged */
      gen_dump(&CurrBest.geno, stdout);
      (void)gen_compile(&CurrBest.geno, x86);
      (void)score(x86, 1);
      printf("No progress for %u generations, trying something new...\n", GEN_DEADEND);
      pop_gen(&Pop, 0, Mutate_Rate); /* start over! */
      gen_since_best = 0;
    }
    generation++;
    gen_since_best++;
  } while (
    !GENOSCORE_MATCH(&CurrBest)
    || CurrBest.geno.len > GEN_PREFIX_LEN + 1 + GEN_SUFFIX_LEN /* shortest possible solution (excluding identity) */
  );
  printf("done.\n");
  (void)gen_compile(&CurrBest.geno, x86);
  (void)score(x86, 1);
  return 0;
}

