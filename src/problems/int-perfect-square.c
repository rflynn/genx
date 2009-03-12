/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * prototype module
 */

#include <stdio.h>
#include <math.h>
#include "typ.h"
#include "gen.h"

static int init(void);
static u32 isPerfectSquare(const u32 []);
static int done(const genoscore *);

static struct {
  u32 in[4],
      out;
} Test[] = {
  { { 0xFFFFFFFF }, 0 },
  { {  0xFFFFFFF }, 0 },
  { {  0x100000A }, 0 },
  { {  0x1000001 }, 0 },
  { {  0x1000000 }, 0 },
  { {     410887 }, 0 },
  { {     410886 }, 0 },
  { {     410885 }, 0 },
  { {     410884 }, 0 },
  { {     410883 }, 0 },
  { {     410882 }, 0 },
  { {     410881 }, 0 },
  { {      10001 }, 0 },
  { {      10000 }, 0 },
  { {       9999 }, 0 },
  { {       2501 }, 0 },
  { {       2500 }, 0 },
  { {       2499 }, 0 },
  { {        101 }, 0 },
  { {        100 }, 0 },
  { {         99 }, 0 },
  { {         38 }, 0 },
  { {         36 }, 0 },
  { {         34 }, 0 },
  { {         25 }, 0 },
  { {         19 }, 0 },
  { {         18 }, 0 },
  { {         17 }, 0 },
  { {         16 }, 0 },
  { {         15 }, 0 },
  { {         14 }, 0 },
  { {         13 }, 0 },
  { {         12 }, 0 },
  { {         11 }, 0 },
  { {         10 }, 0 },
  { {          9 }, 0 },
  { {          8 }, 0 },
  { {          7 }, 0 },
  { {          6 }, 0 },
  { {          5 }, 0 },
  { {          4 }, 0 },
  { {          3 }, 0 },
  { {          2 }, 0 },
  { {          1 }, 0 },
  { {          0 }, 0 },
};

static const struct genx_iface Iface = {
  .test.i = {
    .score = SCORE_ALG,
    .max_const = 0xFFFF,
    .init = init,
    .func = isPerfectSquare,
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

static u32 isPerfectSquare(const u32 x[])
{
  u32 s = (u32)(sqrt(x[0]) + 0.5);
  return s * s == x[0];
}

static int init(void)
{
  for (unsigned i = 0; i < sizeof Test / sizeof Test[0]; i++)
    Test[i].out = isPerfectSquare(Test[i].in);
  return 1;
}

static int done(const genoscore *best)
{
  return
    GENOSCORE_MATCH(best) &&
    best->geno.len <= GEN_PREFIX_LEN + 1 + GEN_SUFFIX_LEN;
}

