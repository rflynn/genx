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

#define POP_SIZE      64 * 1024 /* total genotypes in a population generation */
#define CHROMO_MAX    24        /* maximum chromosomes in a genotype */
#define MAX_CONST     0xFFFF    /* maximum possible random integer value */

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
    u32 score;
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

/* populate prefix */
#define GEN_PREFIX(g) do {              \
    (g)->chromo[0].x86 = ENTER;         \
    (g)->chromo[1].x86 = MOV_8_EBP_EAX; \
  } while (0)

/* populate genotype suffix */
#define GEN_SUFFIX(g) do {              \
  (g)->chromo[(g)->len++].x86 = LEAVE;  \
  (g)->chromo[(g)->len++].x86 = RET;    \
  } while (0)

#endif

