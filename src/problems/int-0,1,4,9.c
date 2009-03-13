/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * the last 4 digits of a perfect square must be 0, 1, 4, or 9
 */

#include <stdio.h>
#include <math.h>
#include "typ.h"
#include "gen.h"

static u32 func(const u32 []);
static int done(const genoscore *);

static struct {
  u32 in[4],
      out;
} Test[] = {
  {{  0 }, 1 },
  {{  1 }, 1 },
  {{  2 }, 0 },
  {{  3 }, 0 },
  {{  4 }, 1 },
  {{  5 }, 0 },
  {{  6 }, 0 },
  {{  7 }, 0 },
  {{  8 }, 0 },
  {{  9 }, 1 },
  {{ 10 }, 0 },
  {{ 11 }, 0 },
  {{ 12 }, 0 },
  {{ 13 }, 0 },
  {{ 14 }, 0 },
  {{ 15 }, 0 }
};

static const struct genx_iface Iface = {
  .test.i = {
    .score = SCORE_ALG,
    .max_const = 0xFFFF,
    .init = NULL,
    .func = func,
    .done = done,
    .data = {
      .len  = sizeof Test / sizeof Test[0],
      .list = &Test
    }
  },
  .opt = {
    .param_cnt      = 1,
		.chromo_min     = 1,
		.chromo_max     = DEFAULT_CHROMO_MAX,
		.pop_size       = DEFAULT_POP_SIZE,
		.pop_keep       = 1,
		.gen_deadend    = 0,
    .mutate_rate    = 0.7,
    .x86 = {
	    .int_ops      = 1,
		  .float_ops    = 0,
		  .algebra_ops  = 1,
		  .bit_ops      = 1,
      .random_const = 1
    }
  }
};

/**
 * interface loading hook
 */
EXPORT const struct genx_iface * load(void)
{
  return &Iface;
}

static u32 func(const u32 x[])
{
  return x[0] == 0
      || x[0] == 1
      || x[0] == 4
      || x[0] == 9;
}

static int done(const genoscore *best)
{
  return
    GENOSCORE_MATCH(best) &&
    best->geno.len <= GEN_PREFIX_LEN + 1 + GEN_SUFFIX_LEN;
}

