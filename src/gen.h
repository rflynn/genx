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
#include <limits.h>
#include "typ.h"

#define DEFAULT_POP_SIZE        64 * 1024 /* total genotypes in a population generation */
#define DEFAULT_CHROMO_MAX      128       /* maximum chromosomes in a genotype */
#define DEFAULT_MAX_INT_CONST   0xFFFF    /* maximum possible random integer value */
#define DEFAULT_MAX_FLT_CONST   10.f      /* max random floating point val */
#define DEFAULT_MIN_FLT_CONST  -10.f      /* min random floating point val */

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
# define GEN_PREFIX_LEN 1
# define GEN_PREFIX(g) do {             \
    (g)->chromo[0].x86 = ENTER;         \
  } while (0)
#endif

#if 0

    (g)->chromo[1].x86 = MOV_8_EBP_EAX; \
    (g)->chromo[2].x86 = MOV_C_EBP_EBX; \
    (g)->chromo[3].x86 = MOV_10_EBP_ECX;\

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

struct genotype {
  u32 len;
  struct op {
    u8 x86,     /* index into X86[] */
       modrm,   /* mod/rm byte, if used */
       data[4]; /* random integer data, if used */
  } *chromo;
};
typedef struct genotype genotype;

void gen_copy(genotype *dst, const genotype *src);

struct pop {
  u32 len;
  struct score_id {
    union sc {
      float f;
      u32   i;
    } score;
    u32 len,
        id;
  } *scores;
  struct genoscore {
    union sc score;
    struct genotype geno;
  } *indiv;
};
typedef struct genoscore genoscore;

void genoscore_copy(genoscore *dst, const genoscore *src);
void gen_dump(const genotype *, FILE *);
u32  gen_compile(genotype *, u8 *, size_t);

int genoscore_lencmp(const void *, const void *);

#ifdef X86_USE_FLOAT
# define GENOSCORE_SCORE(gs)  ((gs)->score.f)
# define GENOSCORE_WORST      FLT_MAX
# define GENOSCORE_BEST       FLT_EPSILON
#else
# define GENOSCORE_SCORE(gs)  ((gs)->score.i)
# define GENOSCORE_WORST      INT_MAX
# define GENOSCORE_BEST       0
#endif

#define GENOSCORE_MATCH(gs)     (GENOSCORE_SCORE(gs) <= GENOSCORE_BEST)
/*
 * better than the worst-possible score
 */
#define GENOSCORE_NOT_WORST(gs) (GENOSCORE_SCORE(gs) < GENOSCORE_WORST)

#define CHROMO_SIZE(iface)    (GEN_PREFIX_LEN + GEN_SUFFIX_LEN + (iface)->opt.chromo_max)

enum scoretype {
  SCORE_BIT,
  SCORE_ALG
};

struct genx_iface {
  union {
    struct {
      enum scoretype score;
      u32 max_const;
      int (*init)(void);
	    u32 (*func)(const u32 []);
	    int (*done)(const genoscore *);
	    struct {
		    const unsigned len;
        const struct {
          u32   in[4],
                out;
        } *list;
      } data;
    } i;
    struct {
      float min_const,
            max_const;
    } f;
	} test;
	struct gen_opts {
		u32      param_cnt,
		         chromo_min, 
		         chromo_max,
		         pop_size,
		         pop_keep;
		u64 		 gen_deadend; 
    double   mutate_rate;
    struct x86_opts {
      unsigned
						  int_ops:1,
						  float_ops:1,
						  algebra_ops:1,
						  bit_ops:1,
              random_const:1;
	  } x86;       
  } opt;
};
typedef struct genx_iface genx_iface;

void pop_score(struct pop *, const genx_iface *, genoscore *tmp);
void pop_gen(struct pop *, u32 keep, const genx_iface *);
             
void genx_iface_dump(const genx_iface *);

#endif

