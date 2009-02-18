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
#define _XOPEN_SOURCE 500 /* drand48() via stdlib */
#include <stdlib.h>
#include "typ.h"
#include "rnd.h"
#include "x86.h"
#include "run.h"

extern const struct x86 X86[X86_COUNT];
extern int Dump;

/**
 * populate g[off..off+len] with random chromosomes
 */
static void chromo_random(genotype *g, unsigned off, unsigned len)
{
  u32 i;
  for (i = off; i < off + len; i++) {
    const struct x86 *x;
#ifdef X86_USE_FLOAT
do_over:
#endif
    g->chromo[i].x86 = randr(X86_FIRST, X86_COUNT - 1);
    assert(g->chromo[i].x86 >= X86_FIRST);
    assert(g->chromo[i].x86 < X86_COUNT);
#ifdef X86_USE_FLOAT
    if (FLD_14EBP == g->chromo[i].x86) {
      /* FIXME: hard-coded logic to handle instruction dependency */
      if (i == off + len - 1) /* need two spaces */
        goto do_over;
      /* generate prerequisite */
      g->chromo[i].x86 = MOV_IMM32_14EBP;
      *(float *)&g->chromo[i].data = randfr(MIN_FLT_CONST, MAX_FLT_CONST);
      i++;
      /* now put real one in */
      g->chromo[i].x86 = FLD_14EBP;
    } else {
#endif
      x = X86 + g->chromo[i].x86;
      g->chromo[i].modrm = gen_modrm(x->modrm);
      if (x->immlen) {
#ifdef X86_USE_FLOAT
        if (FLT == x->flt)
          *(float *)&g->chromo[i].data = randfr(MIN_FLT_CONST, MAX_FLT_CONST);
        else
#endif
          *(u32 *)&g->chromo[i].data = randr(0, MAX_INT_CONST);
      }
#ifdef X86_USE_FLOAT
    }
#endif
  }
}

#define MAX(a,b) ((a)>(b)?(a):(b))

/**
 * randomly mutate a genotype
 */
static void gen_mutate(genotype *g, const double mutate_rate)
{
  /* 
   * NOTE: g->len does NOT include the SUFFIX entries in this
   *       context, but DOES include PREFIX
   */
    /* 
     * original offset; a position within the existing function,
     * must be after the prefix
     *
     * MUST BE ABLE TO BE EQUAL TO G->LEN, IF G IS EMPTY...
     */
  assert(g->len >= GEN_PREFIX_LEN);
  if (RET == g->chromo[g->len-1].x86 || LEAVE == g->chromo[g->len-1].x86) {
    gen_dump(g, stdout);
    abort();
  }

  u32 ooff = GEN_PREFIX_LEN + randr(0, g->len - GEN_PREFIX_LEN - (g->len > GEN_PREFIX_LEN));
  assert(ooff >= GEN_PREFIX_LEN);
  assert(ooff <= g->len);
    /* 
     * original length; number of chromosomes to replace;
     * a value of zero will result in an insertion mutation
     * ooff+olen must be within g->len
     */
  u32 olen = (u32)(rand01() * (g->len - ooff + 1));

  assert(olen <= g->len - GEN_PREFIX_LEN);
  assert(olen <= g->len - ooff);
  assert(ooff + olen <= g->len);
    /* 
     * replacement length; length of chromosomes to generate
     * in place of [ooff..ooff+olen];
     * a result of zero will result in a deletion if olen > 0,
     * else a no-op
     * must fit within CHROMO_MAX
     */
  u32 rlen =
      rand01() *
        ((g->len - (ooff + olen)) + /* space between orig and geno */
         ((GEN_PREFIX_LEN + CHROMO_MAX) > g->len) /* allow growth by one if space */
          + 1); /* rand01 can never == 1.0, so it'll always be truncated, compensate */

  assert(rlen <= g->len - GEN_PREFIX_LEN + 1); /* allow growth by 1 */
  assert(ooff+rlen < GEN_PREFIX_LEN + CHROMO_MAX);
  assert(ooff+(int)(rlen+olen) <= g->len + 1); /* allow growth by 1 */
    /*
     * suffix length; the number of chromosomes after [ooff+olen] which
     * must be shifted to accomodate the 'rlen' elements
     */
  assert(ooff + olen <= g->len);
  assert(ooff + MAX(rlen, olen) <= g->len + 1); /* allow growth by 1 */

  u32 suflen = 0;
  if (ooff < g->len && ooff + MAX(rlen, olen) < g->len)
    suflen = g->len - (ooff + MAX(rlen, olen)); /* data after the mutation */

  assert(ooff + MAX(olen, rlen) + suflen);
  assert(suflen < g->len);

#if 0
  printf("max=%u len=%u ooff+olen=%u leaving window of %u slots\n",
    (GEN_PREFIX_LEN + CHROMO_MAX), g->len, (ooff + olen),
    (g->len-(ooff+olen)) + ((GEN_PREFIX_LEN + CHROMO_MAX) - g->len));
  printf("g->len=%2u ooff=%2u olen=%2u rlen=%2u suflen=%2u\n",
    g->len, ooff, olen, rlen, suflen);
#endif
  if (suflen > 0) {
    /* FIXME: this is corrupting shit! */
#if 0
    printf("%u [%u..%u] <- [%u..%u]\n",
      g->len, ooff+rlen, ooff+rlen+suflen, ooff+olen, ooff+olen+suflen);
#endif
    memmove(g->chromo + ooff + rlen,
            g->chromo + ooff + olen, suflen * sizeof g->chromo[0]);
  }
  if (rlen > 0)
    chromo_random(g, ooff, rlen);
  g->len += (int)(rlen - olen);

  assert(g->len <= GEN_PREFIX_LEN + CHROMO_MAX);
  if (RET == g->chromo[g->len-1].x86 || LEAVE == g->chromo[g->len-1].x86) {
    gen_dump(g, stdout);
    abort();
  }

  assert(g->len <= GEN_PREFIX_LEN + CHROMO_MAX);

  assert(g->chromo[0].x86 < X86_COUNT);
  assert(g->chromo[1].x86 < X86_COUNT);
  assert(g->chromo[2].x86 < X86_COUNT);
  assert(g->chromo[3].x86 < X86_COUNT);

}

static void gen_gen(genotype *dst, const genotype *src, const double mutate_rate)
{
  if (src) {
    /* mutate an existing genotype; by far the most common */
    dst = (genotype *)src;
    dst->len -= GEN_SUFFIX_LEN;
    //GEN_PREFIX(dst);
    gen_mutate(dst, mutate_rate);
  } else {
    /* initial generation or re-generation from scratch, far less common */
    /*
     * always begin genotypes with a single chromosome. simpler
     * solutions are always better, and it is easier to add
     * instructions to a simple partial solution than to remove them
     * from a complex full solution.
     */
    dst->len = GEN_PREFIX_LEN + 1;
    GEN_PREFIX(dst);
    chromo_random(dst, GEN_PREFIX_LEN, dst->len - GEN_PREFIX_LEN);
  }
  GEN_SUFFIX(dst);

  assert(dst->chromo[0].x86 < X86_COUNT);
  assert(dst->chromo[1].x86 < X86_COUNT);
  assert(dst->chromo[2].x86 < X86_COUNT);

}

void pop_gen(struct pop *p, u32 keep, const double mutate_rate)
{
  u32 i;
  if (keep > 0) {
    /*
     * if a set if [0..keep-1] "best" are set, select a random one
     * to serve as the basis for each member of the new generation
     */
    for (i = keep; i < sizeof p->indiv / sizeof p->indiv[0]; i++) {
      const genotype *src = &p->indiv[0].geno;//&p->indiv[randr(0, keep-1)].geno;
      gen_gen(&p->indiv[i].geno, src, mutate_rate);
      GENOSCORE_SCORE(p->indiv+i) = GENOSCORE_MAX;
    }
  } else {
    /*
     * initial generation (or re-generation from scratch), do not
     * use a 'src' element
     */
    for (i = 0; i < sizeof p->indiv / sizeof p->indiv[0]; i++) {
      gen_gen(&p->indiv[i].geno, NULL, mutate_rate);
      GENOSCORE_SCORE(p->indiv+i) = GENOSCORE_MAX;
    }
  }
}

/*
 *
 */
void gen_dump(const struct genotype *g, FILE *f)
{
  char hex[32],
       *h;
  u32 i, j;
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
    fprintf(f, "%3" PRIu32 " %s", i, hex);
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
    memcpy(buf + len, op->data, sizeof op->data);
    len += x->immlen;
  }
  return len;
}

/**
 * calculate the total size of the chromosome in bytes
 */
static u32 chromo_bytes(const struct op *op)
{
  const struct x86 *x = X86 + op->x86;
  u32 len = 0;
  len += x->oplen;
  len += x->modrmlen;
  len += x->immlen;
  return len;
}

/**
 * given an offset, find the closest beginning of a
 * chromosome that we can jump to; we don't want to land in the middle!
 * @param off the total offset of the jump
 * @param rel the relative offset of the jump
 * @param idx the position of the jump within g->indiv, can't jump back!
 */
static s32 gen_jmp_pos(const genotype *g, const u32 off, u32 rel, u32 idx)
{
  u32 total = 0,
      i;
  s32 res;
  assert(idx < g->len);
  assert(idx >= GEN_PREFIX_LEN);
  if (idx == g->len - 1)
    return 0;
  rel = idx + 1 + (rel % (g->len - idx - 2));
  assert(rel < sizeof g->chromo / sizeof g->chromo[0]);
  for (i = 0; i < rel; i++)
    total += chromo_bytes(g->chromo + i);
  res = (s32)(total - off); /* calculate byte offset */
  if (res < 0) /* can't go back, no matter what */
    res = 0;

  assert(res >= 0);

  return res;
}

u32 gen_compile(genotype *g, u8 *buf)
{
  u32 i,
      len = 0;
  for (i = 0; i < g->len; i++) {
    assert(g->chromo[i].x86 < X86_COUNT);
    if (X86[g->chromo[i].x86].jcc) {
      /* if relative jump, adjust the random jump destination to a valid offset */
      *(s32*)&g->chromo[i].data =
        gen_jmp_pos(g, len + chromo_bytes(g->chromo + i),
          (u32)g->chromo[i].data, i);
    }
    len = chromo_add(g->chromo + i, buf, len);
  }
  return len;
}

/**
 * shorter is better, given same score
 */
int genoscore_lencmp(const void *va, const void *vb)
{
  register const genoscore *a = va,
                           *b = vb;
  register int cmp = (GENOSCORE_SCORE(a) > GENOSCORE_SCORE(b))
                   - (GENOSCORE_SCORE(a) < GENOSCORE_SCORE(b));
  if (0 == cmp)
    cmp = (a->geno.len > b->geno.len) - (a->geno.len < b->geno.len);
  return cmp;
}

void pop_score(struct pop *p)
{
  static u8 x86[1024];
  u32 i;
  for (i = 0; i < sizeof p->indiv / sizeof p->indiv[0]; i++) {
    u32 x86len;
    assert(p->indiv[i].geno.chromo[0].x86 < X86_COUNT);
    assert(p->indiv[i].geno.chromo[1].x86 < X86_COUNT);
    assert(p->indiv[i].geno.chromo[2].x86 < X86_COUNT);
    x86len = gen_compile(&p->indiv[i].geno, x86);
    if (Dump > 0)
      x86_dump(x86, x86len, stdout);
    if (Dump > 1)
      (void)gen_dump(&p->indiv[i].geno, stdout);
    GENOSCORE_SCORE(&p->indiv[i]) = score(x86, 0);
  }
  qsort(p->indiv, sizeof p->indiv / sizeof p->indiv[0],
                                    sizeof p->indiv[0], genoscore_lencmp);
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
  g->len = GEN_PREFIX_LEN;
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

