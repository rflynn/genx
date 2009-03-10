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

#if 0
#define POP_SIZE        64 * 1024 /* total genotypes in a population generation */
#define CHROMO_MAX      128       /* maximum chromosomes in a genotype */
#define MAX_INT_CONST   0xFFFF    /* maximum possible random integer value */
#define MAX_FLT_CONST   10.f      /* max random floating point val */
#define MIN_FLT_CONST  -10.f      /* min random floating point val */
#endif

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
# define GEN_PREFIX_LEN 4
# define GEN_PREFIX(g) do {             \
    (g)->chromo[0].x86 = ENTER;         \
    (g)->chromo[1].x86 = MOV_8_EBP_EAX; \
    (g)->chromo[2].x86 = MOV_C_EBP_EBX; \
    (g)->chromo[3].x86 = MOV_10_EBP_ECX;\
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
  struct genoscore {
    union {
      float f;
      u32   i;
    } score;
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

#define GENOSCORE_MATCH(gs)   (GENOSCORE_SCORE(gs) <= GENOSCORE_BEST)

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

void pop_score(struct pop *p, const genx_iface *);
void pop_gen(struct pop *, u32 keep, const genx_iface *);
             
void genx_iface_dump(const genx_iface *);

#endif

