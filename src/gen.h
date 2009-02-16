/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */

#ifndef GEN_H
#define GEN_H

#include <stdio.h>
#include "typ.h"

#define POP_SIZE        64 * 1024 /* total genotypes in a population generation */
#define CHROMO_MAX      64        /* maximum chromosomes in a genotype */
#define MAX_INT_CONST   0xFFFF    /* maximum possible random integer value */
#define MAX_FLT_CONST   10.f      /* max random floating point val */
#define MIN_FLT_CONST  -10.f      /* min random floating point val */

struct genotype {
  u32 len;
  struct op {
    u8 x86,     /* index into X86[] */
       modrm,   /* mod/rm byte, if used */
       data[4]; /* random integer data, if used */
  } chromo[CHROMO_MAX];
};
typedef struct genotype genotype;

struct pop {
  struct genoscore {
    union {
      float f;
      u32   i;
    } score;
    struct genotype geno;
  } indiv[POP_SIZE];
};
typedef struct genoscore genoscore;

void gen_dump(const genotype *g, FILE *f);
u32  gen_compile(genotype *g, u8 *buf);

void pop_gen(struct pop *p,
             u32 keep,
             const double cross_rate,
             const double mutate_rate);

void pop_score(struct pop *p);

#ifdef X86_USE_FLOAT
# define GENOSCORE_SCORE(gs)  ((gs)->score.f)
# define GENOSCORE_MAX        FLT_MAX
# define GENOSCORE_MIN        FLT_EPSILON
#else
# define GENOSCORE_SCORE(gs)  ((gs)->score.i)
# define GENOSCORE_MAX        INT_MAX
# define GENOSCORE_MIN        0
#endif

#define GENOSCORE_MATCH(gs)   (GENOSCORE_SCORE(gs) <= GENOSCORE_MIN)

/*
 * define common op prefix for all functions;
 * required by x86 to set up environment
 */
#ifdef X86_USE_FLOAT
# define GEN_PREFIX_LEN 3
# define GEN_PREFIX(g) do {             \
    (g)->chromo[0].x86 = ENTER;         \
    (g)->chromo[1].x86 = FLD;           \
    (g)->chromo[2].x86 = SUB_14_ESP;    \
  } while (0)
#else
# define GEN_PREFIX_LEN 2
# define GEN_PREFIX(g) do {             \
    (g)->chromo[0].x86 = ENTER;         \
    (g)->chromo[1].x86 = MOV_8_EBP_EAX; \
  } while (0)
#endif

/*
 * common x86 function op suffix
 */
#ifdef X86_USE_FLOAT
# define GEN_SUFFIX_LEN 3
/* populate genotype suffix */
# define GEN_SUFFIX(g) do {                 \
  (g)->chromo[(g)->len++].x86 = ADD_14_ESP; \
  (g)->chromo[(g)->len++].x86 = LEAVE;      \
  (g)->chromo[(g)->len++].x86 = RET;        \
  } while (0)
#else
# define GEN_SUFFIX_LEN 2
/* populate genotype suffix */
# define GEN_SUFFIX(g) do {                 \
  (g)->chromo[(g)->len++].x86 = LEAVE;      \
  (g)->chromo[(g)->len++].x86 = RET;        \
  } while (0)
#endif

/*
 * beginnings of eventual interface describing a problem and
 * providing all parameters necessary to begin an attempt to
 * solve it via genetic algorithm;
 * currently much of this is hard-coded or #define'd
 * we will need a good interface and underlying support for most
 * of it before we can lib-ize genx.
 * how to deal with multiple parameters of varying types is an
 * open question, as is how we will be describe and generate
 * functions for operating on structures in memory.
 */
struct genx_iface_u32 {
	u32 (*func)(u32);
	int (*done)();
	struct {
		unsigned   len;
		u32      (*in_out)[2];
	} target;
	struct {
		u32      param_cnt;     /*
		                         * number of parameters
														 */
		u32      chromo_min,    /*
		                         * Minimum number of chromosomes (x86 operations)
														 * to be considered in a solution.
														 * 
														 * Default: 8
														 * Minimum: 1
														 * Maximum: 64
														 */
		         chromo_max,    /*
		                         * Maximum number of chromosomes (x86 operations)
														 * to be considered in a solution.
														 * 
														 * Default: 32
														 * Minimum: 1
														 * Maximum: 128
														 */
		         gen_size,			/*
		                         * How many genotypes do you wish to produce in
														 * each generation?
														 *
														 * Default: 65536
														 * Minimum: 2
														 * Maximum: 16777216
														 */
		         gen_keep;			/* 
														 * How many of the top genotypes should be used
		                         * to create the next generation?
														 *
														 * Default: 8
														 * Minimum: 1
														 * Maximum: 'gen_size'-1
														 */
		u64 		 gen_deadend;   /* 
		                         * After how many generations do you want to give
		                         * up on mutating the current "best" and re-
														 * generate an entire generation from scratch?
														 *
														 * Default: 10,000
														 * Minimum: 2
														 * Maximum: 0 (never)
														 */
		unsigned minimize_len:1,/*
		                         * Do you consider a genotype with fewer chromosomes
														 * but equivalent functionality to be a better
														 * solution? Obviously it is, but do you want to
														 * spend precious CPU on it?
														 *
														 * Since simply finding a good match is much
														 * more important, and that one can always search
														 * for more canonical mutations of a solution
														 * once one has been settled on, the default action
														 * is to not worry about it.
														 *
														 * Default: 0
														 * Minimum: 0
														 * Maximum: 1
														 */
						 int_ops:1,			/*
						                 * Do you want to consider
														 */
						 float_ops:1,
						 algebra_ops:1,
						 bit_ops:1,
             random_const:1;/*
                             * Are we allowed to generate random constant values
                             * or must we stick with only the input given to us?
                             */
	} opt;       
};

#endif

