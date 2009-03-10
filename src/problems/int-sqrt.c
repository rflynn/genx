/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * prototype module
 */

#include <stdio.h>
#include <math.h>
#include "typ.h"
#include "gen.h"

static u32 func(const u32 []);
static int done(const genoscore *);

static const struct {
  u32 in[4],
      out;
} Test[] = {
  { { 0xFFFFFFFF }, 0x10000 },
  { {  0xFFFFFFF },  0x3fff },
  { {  0x1000000 },  0x1000 },
  { {      10000 },     100 },
  { {       2500 },      50 },
  { {        100 },      10 },
  { {          9 },       3 },
  { {          4 },       2 },
  { {          2 },       1 },
  { {          1 },       1 },
  { {          0 },       0 },
};

static const struct genx_iface Iface = {
  .test.i = {
    .score = SCORE_ALG,
    .max_const = 0xFFFF,
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
		.chromo_max     = 128,
		.pop_size       = 64 * 1024,
		.pop_keep       = 2,
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

EXPORT void init(void)
{
}

static u32 func(const u32 x[])
{
  return (s32)sqrt(x[0]);
}

static int done(const genoscore *best)
{
  return
    GENOSCORE_MATCH(best) &&
    best->geno.len <= GEN_PREFIX_LEN + 1 + GEN_SUFFIX_LEN;
}

