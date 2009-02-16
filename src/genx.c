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
#include "run.h"

#define GEN_DEADEND   1000      /* start over after this many generations of no progress */
#define POP_KEEP      3         /* keep this many from generation to generation */
#define MINIMIZE_LEN  1         /* do we care about finding the shortest solution, or just any match? */

extern const struct x86 X86[X86_COUNT];
extern int genoscore_cmp(const void *, const void *);

/******************* BEGIN INPUT PART *****************************/

#if 0
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
#endif

static s32 magic(s32 x)
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
  float  y = *(float *)&x;  /* type-pun int->float */
  float  f = 1.f/sqrtf(y);  /* calculate true value */
  s32    s = *(s32 *)&f;    /* type-pun back */
  return s;
}

struct target Target[256] = {
  {      ~0, 0 },
  { INT_MAX, 0 },
  {   65535, 0 },
  {     255, 0 },
  {       2, 0 },
  {       1, 0 },
  {       0, 0 },
};
u32 TargetLen = 6;

/********************* END INPUT PART *****************************/

static void calc_target(void)
{
  unsigned i;
  printf("TargetLen <- %u\n", TargetLen);
  for (i = 0; i < TargetLen; i++) {
    Target[i].out = magic(Target[i].in);
    printf("Target %3u %11" PRIt " -> %11" PRIt "\n",
      i, Target[i].in, Target[i].out);
  }
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
  printf("FLT_EPSILON=%f\n", FLT_EPSILON);
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
  calc_target();
  GENOSCORE_SCORE(&CurrBest) = GENOSCORE_MAX;
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
    progress = -1 == genoscore_cmp(&Pop.indiv[0], &CurrBest);
    if (progress || 0 == generation % 100) {
      /* display generation regularly or on progress */
      time_t t = time(NULL);
      printf("GENERATION %5" PRIu32 " %10" PRIu64 " genotypes @%s",
        generation, indivs, ctime(&t));
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
      pop_gen(&Pop, POP_KEEP, Cross_Rate, Mutate_Rate); /* retain best from previous */
    } else {
      /* we have run GEN_DEADEND generations and haven't made any progress;
       * throw out our top genetic material and start fresh, but still
       * retain CurrBest, against which all new candidates are judged */
      gen_dump(&CurrBest.geno, stdout);
      (void)gen_compile(&CurrBest.geno, x86);
      (void)score(x86, 1);
      printf("No progress for %u generations, trying something new...\n", GEN_DEADEND);
      pop_gen(&Pop, 0, Cross_Rate, Mutate_Rate); /* start over! */
      gen_since_best = 0;
    }
    generation++;
    gen_since_best++;
  } while (!GENOSCORE_MATCH(&CurrBest));
  printf("done.\n");
  (void)gen_compile(&CurrBest.geno, x86);
  (void)score(x86, 1);
  return 0;
}

