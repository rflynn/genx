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
#include "typ.h"
#include "rnd.h"
#include "x86.h"

extern const struct x86 X86[X86_COUNT];
extern int Dump;
extern float score(func f, int verbose);

static void chromo_random(genotype *g, unsigned off, unsigned len)
{
  u32 i;
  for (i = off; i < off + len; i++) {
    const struct x86 *x;
    g->chromo[i].x86 = randr(X86_FIRST, X86_COUNT - 1);
    x = X86 + g->chromo[i].x86;
    g->chromo[i].modrm = gen_modrm(x->modrm);
    if (x->immlen) {
      //*(u32 *)&g->chromo[i].data = randr(0, MAX_CONST);
      *(float *)&g->chromo[i].data = randfr(MIN_FLT_CONST, MAX_FLT_CONST);
    }
  }
}

static void gen_mutate(genotype *g, const double mutate_rate)
{
  /* calculate an offset within g for a mutation occur;
   * calculate the length of the area to be mutated;
   * calculate the length of the replacement;
   * this allows for insertions, deletions and replacements
   */
  u32 doff   = randr(FUNC_PREFIX_LEN, FUNC_PREFIX_LEN + g->len - FUNC_SUFFIX_LEN),
      /* NOTE: incorporate mutate_rate somehow! */
      dlen   = (u32)(rand01() * (g->len - doff)),
      slen   = (u32)(rand01() *
        (CHROMO_MAX - FUNC_SUFFIX_LEN - (g->len - dlen - 1))),
      suflen = g->len - (doff + dlen); /* data after the mutation */
  assert(g->len + (int)(slen - dlen) <= CHROMO_MAX - FUNC_SUFFIX_LEN);
  if (suflen > 0)
    memmove(g->chromo + doff + slen,
            g->chromo + doff, suflen * sizeof g->chromo[0]);
  if (slen > 0)
    chromo_random(g, doff, slen);
  g->len += (int)(slen - dlen);
  assert(g->len <= CHROMO_MAX - FUNC_SUFFIX_LEN);
}

static void gen_gen(genotype *dst, const genotype *src,
                    const double cross_rate, const double mutate_rate)
{
  if (src) {
    /* mutate an existing genotype */
    memcpy(dst, src, sizeof *dst);
    dst->len -= FUNC_SUFFIX_LEN;
    gen_mutate(dst, mutate_rate);
  } else {
    /* initial generation */
    dst->len = randr(FUNC_PREFIX_LEN + 1, CHROMO_MAX - FUNC_SUFFIX_LEN);
    chromo_random(dst, FUNC_PREFIX_LEN, dst->len);
    GEN_PREFIX(dst);
  }
  GEN_SUFFIX(dst);
}

void pop_gen(struct pop *p,
             u32 keep,
             const double cross_rate,
             const double mutate_rate)
{
  u32 i;
  if (keep > 0) {
    /*
     * if a set if [0..keep-1] "best" are set, select a random one
     * to serve as the basis for each member of the new generation
     */
    for (i = keep; i < sizeof p->indiv / sizeof p->indiv[0]; i++) {
      const genotype *src = &p->indiv[randr(0, keep-1)].geno;
      gen_gen(&p->indiv[i].geno, src, cross_rate, mutate_rate);
      p->indiv[i].score = 0;
    }
  } else {
    /*
     * initial generation, calculate completely random
     */
    for (i = keep; i < sizeof p->indiv / sizeof p->indiv[0]; i++) {
      gen_gen(&p->indiv[i].geno, NULL, cross_rate, mutate_rate);
      p->indiv[i].score = 0;
    }
  }
}

void gen_dump(const struct genotype *g, FILE *f)
{
  char hex[32],
       *h;
  u32 i, j;
  assert(g->len <= CHROMO_MAX);
  for (i = 0; i < g->len; i++) {
    const struct x86 *x = X86 + g->chromo[i].x86;
    h = hex;
    for (j = 0; j < x->oplen; j++) {
      sprintf(h, "%02" PRIx8 " ", x->op[j]);
      h += 3;
    }
    if (x->modrmlen) {
      sprintf(h, "%02" PRIx8 " ", g->chromo[i].modrm);
      h += 3;
    }
    for (j = 0; j < x->immlen; j++) {
      sprintf(h, "%02" PRIx8 " ", g->chromo[i].data[j]);
      h += 3;
    }
    memset(h, ' ', 32 - (h - hex));
    h += 32 - (h - hex);
    *h = '\0';
    fprintf(f, "%2" PRIu32 " %s", i, hex);
    fprintf(f, x->descr, *(u32 *)&g->chromo[i].data);
    if (x->modrmlen) {
      char modbuf[16];
      fprintf(f, " %s", disp_modrm(g->chromo[i].modrm, x->modrm, modbuf, sizeof modbuf));
    }
    fputc('\n', f);
  }
}

/**
 * append a command to the buffer
 */
inline static u32 chromo_add(const struct op *op, u8 *buf, u32 len)
{
  const struct x86 *x = X86 + op->x86;
#if 0
  assert(op->x86     < sizeof X86 / sizeof X86[0]);
  assert(x->oplen  <= 4);
  assert(x->modrmlen <= 1);
  assert(x->immlen  <= 4);
#endif
  //memcpy(buf + len, x->op, x->oplen);
  /* maximum possible value, but not always correc,t why use it?
   * using a constant value for length lets optimizing compilers
   * remove the memcpy call altogether, and it still works because
   * we simply overwrite and extraneous bytes
   */
  memcpy(buf + len, x->op, sizeof x->op); 
  len += x->oplen;
  if (x->modrmlen)
    buf[len++] = op->modrm;
  if (x->immlen) {
    //memcpy(buf + len, op->data, x->immlen);
    memcpy(buf + len, op->data, sizeof op->data);
    len += x->immlen;
  }
  return len;
}

u32 gen_compile(genotype *g, u8 *buf)
{
  u32 i,
      len = 0;
  for (i = 0; i < g->len; i++)
    len = chromo_add(g->chromo + i, buf, len);
  return len;
}

/**
 * given 2 genoscores, find the one with the lowest (closest) score.
 * if the scores are identical, favor the shorter one.
 */
static int genoscore_cmp(const void *va, const void *vb)
{
  const genoscore *a = va,
                  *b = vb;
  return (a->score > b->score) - (a->score < b->score);
}

static int genoscore_lencmp(const void *va, const void *vb)
{
  const genoscore *a = va,
                  *b = vb;
  if (a->score == b->score) /* shorter is better, given same score */
    return (a->geno.len > b->geno.len) - (a->geno.len < b->geno.len);
  return (a->score > b->score) - (a->score < b->score);
}

void pop_score(struct pop *p)
{
  static u8 x86[1024];
  u32 i;
  for (i = 0; i < sizeof p->indiv / sizeof p->indiv[0]; i++) {
    u32 x86len = gen_compile(&p->indiv[i].geno, x86);
    if (Dump > 0)
      x86_dump(x86, x86len, stdout);
    if (Dump > 1)
      (void)gen_dump(&p->indiv[i].geno, stdout);
    p->indiv[i].score = score((func)x86, 0);
    assert(p->indiv[0].score > 0.f);
  }
  qsort(p->indiv, sizeof p->indiv / sizeof p->indiv[0],
                                    sizeof p->indiv[0], genoscore_cmp);
}

#if 0
/**
 * load a genotype representation from gen_dump()
 */
static int gen_load(FILE *f, genotype *g)
{
  char line[256];
  u8   byte[8],
       tmp;
  char descr[16];
  int  tmp;
  struct op *o;
  /* setup prefix */
  GEN_PREFIX(g);
  g->len = FUNC_PREFIX_LEN;
  /* read genotype from file */
  while (NULL != fgets(line, sizeof line[0], f)) {
    if (
         8 == sscanf("%2hhu %02" PRIx8 " %02" PRIx8 " %02" PRIx8 
                          " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %.*s",
                      &tmp, byte+0, byte+1, byte+2,
                            byte+3, byte+4, byte+5, (int)(sizeof descr) - 1, descr)) {
      o = g->chromo + g->len;
      o->x86 = x86_by_descr(descr);
      if (X86_NOTFOUND == o->x86) {
        fprintf(stderr, "Could not locate x86 instruction '%s'!\n", descr);
        return 0;
      } else {
        const x86 *x = X86 + o->x86;
        unsigned boff = x->oplen;
        if (x->modrmlen)
          o->modrm = byte[boff++];
        if (x->immlen)
          memcpy(o->data, byte+boff, x->immlen);
      }
      g->len++;
    }
  }
  GEN_SUFFIX(g);
  return 1;
}
#endif


#if 0
static void test_load(void)
{
  genotype g;
  FILE *f = fopen("success-sqrt.txt", "r");
  gen_load(f, &g);
  printf("loaded:\n");
  gen_dump(&g, stdout);
}
#endif

