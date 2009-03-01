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

#define GEN_DEADEND 10000    /* start over after this many generations of no progress */
                             /*
                              * FIXME: this is the right idea, but a bad implementation
                              * of it. for any given solution, there will be some
                              * minimum solution that a single mutation is extremely
                              * likely to improve.
                              *
                              * after a certain period of non-progress we should be
                              * able to store/push a current solution and start over
                              * again fresh.
                              *
                              * the reason this won't work now is because we always
                              * keep that solution in CurrBest and compare everything
                              * to it; which prevents us from re-growing a new one,
                              * since starting from scratch means even the best
                              * solution will not be perfect from the start (and
                              * which will be discarded in favor of the longer but
                              * technically better solutions)
                              *
                              */

#define POP_KEEP        1    /* keep this many from generation to generation */

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

#ifdef X86_USE_INT
/* test multi-parameter */
static s32 magic(s32 x[])
{
#if 0 /* found */
  return x[0] + x[1];
  return x[0] * x[1];
  return (x[0] * x[1]) + x[2];
  return 0x55555555 ^ x[0]; /* ha! found equivalence very quickly, i'm surprised */
  return x[0] / x[1]; // found pretty quickly
  return x[0] / (((x[2] * x[1]) / x[0]) + 1); /* found, not as quickly */
  return x[0] ^ (x[1] << 16) ^ (x[2] >> 3);
  return (x[0] > 0) - (x[0] < 0); /* return 1 for positive, 0 for zero, -1 for negative */
  return x[0]+1; /* double-check that we can find a solution that contains
                  * EXACTLY one instruction */
  return x[0]+2;
  return x[0]+x[1]+1;
  return x[0]+3; /* double-check that we can find a solution that contains
                  * EXACTLY one instruction */
#endif

/* I don't even know why this works, but it is pretty short :/
GENERATION      53    3538944 genotypes (321722.2/sec) @Wed Feb 18 02:08:06 2009
  0 c8 00 00 00                     enter
  1 8b 45 08                        mov     0x8(%ebp), %eax
  2 8b 5d 0c                        mov     0xc(%ebp), %ebx
  3 8b 4d 10                        mov     0x10(%ebp), %ecx
  4 c1 d8 22                        rol     0x22, %ebx, %eax
  5 c1 e8 1f                        shr     0x1f, %eax
  6 0f ba f8 4e                     btc     0x4e, %eax
  7 81 f0 ff bf 00 00               xor     0x0000bfff, %eax
  8 c9                              leave
  9 c3                              ret
->score=0
  return 0xffff ^ (x[0] & 1);
*/

  return (s32)sqrt(x[0]);
  
#if 0 /* not found: */
  return 0x55555555 | x[0]; // not found yet
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
#endif
}

#endif

/*
 * target input -> output data
 * this is what we test against
 */
struct target Target[] = {
#ifdef X86_USE_FLOAT
  /*
   * age, co2, temp data recorded from ice cores by
   * the Vostok Antarctic research station
   * @ref ftp://cdiac.ornl.gov/pub/trends/co2/vostok.icecore.co2
   * @ref http://cdiac.esd.ornl.gov/ftp/trends/temp/vostok/vostok.1999.temp.dat
   */
  /* air        co2            temp
   * age        ppmv           var (C) */
  {{   2342.f,  284.7f, 0.f }, -0.98f },
  {{  50610.f,  189.3f, 0.f }, -5.57f },
  {{ 103733.f,  225.9f, 0.f }, -4.50f },
  {{ 151234.f,  197.0f, 0.f }, -6.91f },
  {{ 201324.f,  226.4f, 0.f }, -1.49f },
  {{ 401423.f,  277.1f, 0.f }, -1.09f },
#else
  /* a range of integer data with which to play */
  //{{         -1, 0, 0 }, 0 },
  //{{ 0xffffffff, 0, 0 }, 0 },
  {{ 0x7fffffff, 0, 0 }, 0 },
  {{ 0x30305123, 0, 0 }, 0 },
  {{ 0x12345678, 0, 0 }, 0 },
  {{  0xeeeeeee, 0, 0 }, 0 },
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
s32 TargetSum = 0;

/********************* END INPUT PART *****************************/

#ifdef X86_USE_INT
static void calc_target(void)
{
  unsigned i;
  printf("TargetLen <- %u\n", TargetLen);
  for (i = 0; i < TargetLen; i++) {
    Target[i].out = magic(Target[i].in);
    TargetSum += Target[i].out;
    printf("Target %3u (%11" PRIt ",%11" PRIt ",%11" PRIt ") -> %11" PRIt "\n",
      i, Target[i].in[0], Target[i].in[1], Target[i].in[2], Target[i].out);
  }
  TargetSum = abs(TargetSum);
}
#endif

int Dump = 0; /* verbosity level */

static u8 *x86;
static u32 x86len;

static void genoscore_exec(genoscore *g)
{
  if (0 == g->geno.len) {
    printf("genoscore empty, you suck\n");
  } else {
    x86len = gen_compile(&g->geno, x86);
    assert(score(x86, 1) == GENOSCORE_SCORE(g));
  }
}

int main(int argc, char *argv[])
{
  static struct pop Pop;
  /* mutation rates */
  const double Mutate_Rate    = 0.2;  /* [0,1] larger values promote more
                                       * more radical mutations */
  time_t       Start;
  genoscore    CurrBest;              /* save best found solution */
  u64          indivs         = 0;    /* total creatures created; status */
  u32          generation     = 0,    /* track time; status */
               gen_since_best = 0;    /* dead end counter */
  x86 = malloc(1024);
  printf("x86=%p\n", (void *)x86);
  assert(NULL != x86);
  /* show sizes of our core types in bytes */
  printf("sizeof Pop.indiv[0]=%lu\n", (unsigned long)(sizeof Pop.indiv[0]));
  printf("sizeof Pop=%lu\n", (unsigned long)(sizeof Pop));
  printf("FLT_EPSILON=%g\n", FLT_EPSILON);
  /* sanity-checks */
  assert(CHROMO_MAX > 0);
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

  /* sanity check, initialization checks */
  randr_test();

  memset(&Pop, 0, sizeof Pop);
#ifndef WIN32
  nice(+19); /* be as polite to any other programs as possible */
#endif
  Start = time(NULL);
  printf("Start=%lu\n", (unsigned long)Start);

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
      printf("GENERATION %7" PRIu32 " %10" PRIu64 " genotypes (%.1f/sec) @%s",
        generation, indivs, (double)indivs / (t - Start + 0.0001), ctime(&t));
      if (progress) {
        /* only display best if it's changed; raises signal/noise ration */
        CurrBest = Pop.indiv[0];
        gen_dump(&CurrBest.geno, stdout);
        printf("->score=%" PRIt "\n", GENOSCORE_SCORE(&Pop.indiv[0]));
        /* show detailed score from best candidate */
        genoscore_exec(&CurrBest);
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
      genoscore_exec(&CurrBest);
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
  genoscore_exec(&CurrBest);
  return 0;
}

