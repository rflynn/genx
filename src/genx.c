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
#include <errno.h>
#include <dlfcn.h> /* dlopen */
#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include "typ.h"
#include "rnd.h"
#include "x86.h"
#include "gen.h"
#include "run.h"

#if 0
#define POP_KEEP        1    /* keep this many from generation to generation */
#endif

extern const struct x86 X86[X86_COUNT];

/******************* BEGIN INPUT PART *****************************/

#if 0
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
#endif

/********************* END INPUT PART *****************************/

#if 0
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
#endif

int Dump = 0; /* verbosity level */


static void *Iface_Handle = NULL;
struct genx_iface *Iface = NULL;

static struct genx_iface * load_module(const char *name)
{
  char path[512];
  snprintf(path, sizeof path, "%s%s%s", "problems/", name, MODULE_EXTENSION);
  printf("module '%s' -> '%s'\n", name, path);
  errno = 0;
  Iface_Handle = dlopen(path, RTLD_NOW);
  if (NULL == Iface_Handle) {
    perror("dlopen");
  } else {
    void *sym = dlsym(Iface_Handle, "load");
    if (NULL == sym) {
      perror("dlsym");
      dlclose(Iface_Handle);
    } else {
      Iface = ((struct genx_iface *(*)())sym)();
    }
  }
  return Iface;
}

static void unload_module(void *handle)
{
  dlclose(handle);
}

static void pop_init(struct pop *p, const struct genx_iface *iface)
{
  p->len = iface->opt.gen_size;
  p->indiv = malloc(sizeof *p->indiv * p->len);
  assert(p->indiv != NULL);
  memset(p->indiv, 0, sizeof *p->indiv * p->len);
}

int main(int argc, char *argv[])
{
  static struct pop Pop;
  time_t       Start;
  genoscore    CurrBest;              /* save best found solution */
  u64          indivs         = 0;    /* total creatures created; status */
  u32          generation     = 0;    /* track time; status */
  /* show sizes of our core types in bytes */
  printf("sizeof Pop.indiv[0]=%lu\n", (unsigned long)(sizeof Pop.indiv[0]));
  printf("sizeof Pop=%lu\n", (unsigned long)(sizeof Pop));
  printf("FLT_EPSILON=%g\n", FLT_EPSILON);
  /* sanity-checks */
  assert(Iface->opt.chromo_max > 0);
  /* one-time initialization */
  x86_init();
  if (argc > 1) {
    Dump += 'd' == argv[1][1];
    Dump += (2 * ('D' == argv[1][1]));
  }
  /* TODO: call interface init function */
  GENOSCORE_SCORE(&CurrBest) = GENOSCORE_MAX;
  CurrBest.geno.len = 0;
  rnd32_init((u32)time(NULL));
  setvbuf(stdout, (char *)NULL, _IONBF, 0); /* unbuffer stdout */

  /* sanity check, initialization checks */
  randr_test();

  pop_init(&Pop, Iface);
#ifndef WIN32
  nice(+19); /* be as polite to any other programs as possible */
#endif
  Start = time(NULL);
  printf("Start=%lu\n", (unsigned long)Start);

  pop_gen(&Pop, 0, Iface->opt.mutate_rate); /* seminal generation */
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
        score(&CurrBest, 1);
      }
    }
    pop_gen(&Pop, Iface->opt.gen_keep, Iface->opt.mutate_rate); /* retain best from previous */
    generation++;
  } while (!(*Iface->test.i.done)(&CurrBest));
  printf("done.\n");
  score(&CurrBest, 1);
  return 0;
}

