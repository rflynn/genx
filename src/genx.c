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

/******************* BEGIN INPUT PART *****************************/

#if 0
/*
 * target input -> output data
 * this is what we test against
 */
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

/********************* END INPUT PART *****************************/
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
    fprintf(stderr, dlerror());
    fputc('\n', stderr);
  } else {
    void *sym = dlsym(Iface_Handle, "load");
    if (NULL == sym) {
      fprintf(stderr, dlerror());
      fputc('\n', stderr);
      dlclose(Iface_Handle);
    } else {
      Iface = ((struct genx_iface *(*)())sym)(); /* fucking ISO C... */
    }
  }
  return Iface;
}

static void * unload_module(void *handle)
{
  dlclose(handle);
  return NULL;
}

/**
 * allocate and initialize 
 */
static void pop_init(struct pop *p, const genx_iface *iface)
{
  p->len = iface->opt.pop_size;
  p->indiv = malloc(p->len * sizeof p->indiv[0]);
  assert(p->indiv != NULL);
  p->indiv[0].geno.len = 0;
  /* enough space for all chromosomes for entire pop to [0] */
  p->indiv[0].geno.chromo = malloc(p->len *
    (CHROMO_SIZE(iface) * sizeof(struct op)));
  assert(p->indiv[0].geno.chromo && "Use smaller pop_size, fewer chromo_max or buy more RAM");
  /* initialize all indivs */
  for (u32 i = 1; i < p->len; i++) {
    p->indiv[i].geno.len = 0;
    /* assign each individual a slice of the whole contiguous memory vector */
    p->indiv[i].geno.chromo = p->indiv[i-1].geno.chromo + CHROMO_SIZE(iface);
    //p->indiv[i].geno.chromo = malloc(CHROMO_SIZE(iface) * sizeof(struct op));
  }
  p->scores = malloc(iface->opt.pop_size * sizeof *p->scores);
  assert(p->scores);
}

static void evolve(
        genoscore  *best,
        genoscore  *tmp,
        struct pop *pop,
  const genx_iface *iface,
  const time_t      start)
{
  u32 gencnt = 0;
  GENOSCORE_SCORE(best) = GENOSCORE_WORST;
  best->geno.len = 0;
  pop_gen(pop, 0, iface);
  do {
    int progress;
    pop_score(pop, iface, tmp);
    progress = -1 == genoscore_lencmp(pop->indiv, best);
    if (progress || 0 == gencnt % 1000) { /* display generation regularly or on progress */
      u64 indivs = iface->opt.pop_size * (gencnt + 1);
      time_t t = time(NULL);
      printf("GENERATION %7" PRIu32 " %10" PRIu64 " genotypes (%.1fk/sec) @%s",
        gencnt, indivs, (double)indivs / (t - start + 1.) / 1000., ctime(&t));
      if (progress) {
        genoscore_copy(best, &pop->indiv[0]);
        gen_dump(&best->geno, stdout);
        printf("->score=%" PRIt "\n", GENOSCORE_SCORE(pop->indiv));
        score(best, iface, 1);
      }
    }
    pop_gen(pop, iface->opt.pop_keep, iface);
    gencnt++;
  } while (!(*iface->test.i.done)(best)
  );
}

int main(int argc, char *argv[])
{
  struct pop Pop;
  genoscore  Best,  /* best function so far */
             Tmp;   /* swap space for sorting/swapping */
  time_t     Start;
  int        mod_idx = 1; /* argv[mod_idx] is name of module */

  /* sanity check */
  printf("sizeof Pop.indiv[0]=%lu\n", (unsigned long)(sizeof Pop.indiv[0]));
  printf("sizeof Pop=%lu\n", (unsigned long)(sizeof Pop));
  printf("FLT_EPSILON=%g\n", FLT_EPSILON);

  if (argc > 1) {
    if (0 == strcmp("-d", argv[1])) {
      Dump = 1;
    } else if (0 == strcmp("-D", argv[1])) {
      Dump = 2;
    }
    mod_idx += !!Dump;
  }

  if (argc < mod_idx) {
    printf("Usage: genx <module>\n"
           "(where problems/<module>.%s exists)\n",
           MODULE_EXTENSION);
    exit(EXIT_FAILURE);
  }

  /* initialization */
  Iface = load_module(argv[mod_idx]);
  assert(Iface);
  assert(Iface_Handle);
  assert(Iface->opt.chromo_max > 0);
  assert(Iface->opt.pop_keep < Iface->opt.pop_size && "wtf are you doing");
  assert(Iface->test.i.data.len > 0);
  genx_iface_dump(Iface);
  printf("CHROMO_SIZE(%p)..%u\n", (void*)Iface, CHROMO_SIZE(Iface));
  printf("sizeof(struct op)..%u\n", (unsigned)sizeof(struct op));
  printf("sizeof chromosome..%u\n", (unsigned)(CHROMO_SIZE(Iface)*sizeof(struct op)));
  printf("sizeof Pop.indiv[0]..%u\n", (unsigned)sizeof Pop.indiv[0]);
  /* call Iface init */
  if (NULL != Iface->test.i.init) {
    printf("Iface.init...");
    fflush(stdout);
    if (0 == (*Iface->test.i.init)()) {
      printf("failed!\n");
      exit(EXIT_FAILURE);
    } else {
      printf("OK\n");
    }
  }

  Best.geno.len = 0;
  Best.geno.chromo = malloc(CHROMO_SIZE(Iface) * sizeof(struct op));

  Tmp.geno.chromo = malloc(CHROMO_SIZE(Iface) * sizeof(struct op));
  x86_init();
  run_init();
  rnd32_init((u32)time(NULL));
  randr_test();
#ifndef WIN32
  nice(+19); /* be as polite to any other programs as possible */
#endif
  pop_init(&Pop, Iface);
  Start = time(NULL);
  printf("Start=%lu\n", (unsigned long)Start);

  evolve(&Best, &Tmp, &Pop, Iface, Start);

  printf("done.\n");
  score(&Best, Iface, 1);
  Iface = unload_module(Iface_Handle);

  return 0;
}

