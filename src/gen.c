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
 * from X86
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

static void gen_mutate(genotype *g, const double mutate_rate)
{
  u32 ooff,
      olen,
      rlen,
      suflen;
  s32 difflen;

/*
 * randomly mutate a genotype
 *
 * a genotype passed to us has already been populated with
 * a prefix, a suffix and chromosomes in-between.
 *
 *    GEN_PREFIX_LEN                         GEN_PREFIX_LEN + CHROMO_MAX
 *          v                                               v
 * +--------------------------------------------------------+
 * | prefix |         c h r o m o s o m e s        | suffix |
 * +--------------------------------------------------------+
 *                                                 ^
 * g->len falls within the range GEN_PREFIX_LEN+(0,CHROMO_MAX).
 * g->len may be modified by (-1, 0, +1) each time, as long as it
 * stays within the proper bounds.
 */

#ifdef DEBUG
  assert(g->len >= GEN_PREFIX_LEN);
  assert(g->len <= GEN_PREFIX_LEN + CHROMO_MAX);
#endif

/*
 *
 * to perform the mutation we select "ooff", a random "original" offset within
 * the range (GEN_PREFIX_LEN, g->len):
 *
 *
 *          |-------------- ooff range ------------|
 * +--------------------------------------------------------+
 * | prefix |         c h r o m o s o m e s        | suffix |
 * +--------------------------------------------------------+
 *                                                 ^
 *                                               g->len
 */

  ooff = randr(GEN_PREFIX_LEN, g->len);
#ifdef DEBUG
  assert(ooff >= GEN_PREFIX_LEN);
  assert(ooff <= g->len);
#endif

/*
 * then, based on ooff we need to select a number of chromosomes
 * to constitute a range, this must be in the range (0, g->len - ooff),
 * this is "olen"
 *
 *                             |--- olen range ----|
 * +--------------------------------------------------------+
 * | prefix |         c h r o m o s o m e s        | suffix |
 * +--------------------------------------------------------+
 *                             ^                   ^
 *                            ooff               g->len
 */

  olen = randr(0, g->len - ooff);
#ifdef DEBUG
  assert(olen <= g->len);
  assert(olen <= g->len - ooff);
#endif

/*
 * now we must select an "rlen", a replacement length. rlen
 * must be in the range:
 * (g->len < GEN_PREFIX + CHROMO_MAX) + (0, g->len - ooff)
 *
 * NOTE this allows ooff+rlen to be == g->len+1 iff g->len is
 * not already the maximum length.
 *
 * depending on the relative lengths of rlen and olen we can
 * simulate different mutative operations:
 *
 * olen = 0 and rlen = 0: no-op
 * olen = 0 and rlen > 0: insertion
 * olen > 0 and rlen = 0: deletion
 * otherwise: normal mutation, possibly shortening or lengthening
 * the gene.
 *
 *                                  rlen range
 *                                     |-|
 * +--------------------------------------------------------+
 * | prefix |         c h r o m o s o m e s        | suffix |
 * +--------------------------------------------------------+
 *                             ^        ^          ^
 *                            ooff   ooff+olen   g->len
 *
 * rlen's calculate must consider that the difference in rlen - olen
 * cannot be greater than the difference between g->len and GEN_PREFIX_LEN + CHROMO_MAX,
 * as it would push the suffix too far!
 */

#ifdef DEBUG
  assert(g->len - ooff < g->len);
  assert(g->len - ooff - olen < g->len);
#endif

  rlen = randr(olen - ((olen > 0) && (g->len > GEN_PREFIX_LEN)),
               olen + (g->len < GEN_PREFIX_LEN + CHROMO_MAX));

#ifdef DEBUG
  assert(rlen <= CHROMO_MAX);
  assert(rlen <= g->len + 1);
  assert(rlen <= g->len + (g->len < GEN_PREFIX_LEN + CHROMO_MAX));
  assert(g->len - ooff + (int)(rlen - olen) < GEN_PREFIX_LEN + CHROMO_MAX);
  assert(ooff + MAX(olen, rlen) <= g->len + 1);
#endif

/*
 * the last piece of the puzzle is 'suflen', the length of the
 * chromosomes after 'olen' that will need to be shifted by
 * (rlen - olen) places to preserve the existing chromosomes;
 * it will be exactly:
 * g->len - (ooff + rlen + (ooff + olen == g->len + 1))
 * because the length is allowed to grow by one if not already
 * the maximum length
 *
 *                                     ooff+rlen
 *                                       v           
 * +--------------------------------------------------------+
 * | prefix |         c h r o m o s o m e s        | suffix |
 * +--------------------------------------------------------+
 *                             ^        ^          ^
 *                            ooff   ooff+olen   g->len
 */

  suflen = g->len - (ooff + olen);

#ifdef DEBUG
  assert(suflen <= g->len);
  assert(suflen <= ooff + olen + g->len);
  assert(ooff + olen + suflen == g->len);
#endif

  difflen = (s32)(rlen - olen);

#if 0
  printf("g->len=%u ooff=%u olen=%u rlen=%u difflen=%d suflen=%u\n",
    g->len, ooff, olen, rlen, difflen, suflen);
#endif

  if (suflen > 0 && olen != rlen) {
    memmove(g->chromo + ooff + rlen,
            g->chromo + ooff + olen, suflen * sizeof g->chromo[0]);
  }

  if (rlen > 0) {
    chromo_random(g, ooff, rlen);
  }

#ifdef DEBUG
  assert(difflen <= (s32)g->len);
  assert(g->len + difflen <= GEN_PREFIX_LEN + CHROMO_MAX);
  assert(abs(difflen) <= (s32)g->len);
  assert(abs(difflen) <= (s32)g->len - GEN_PREFIX_LEN + 1);
#endif

  g->len += difflen;

#ifdef DEBUG
  assert(g->len >= GEN_PREFIX_LEN);
  assert(g->len <= GEN_PREFIX_LEN + CHROMO_MAX);

  assert(g->len <= GEN_PREFIX_LEN + CHROMO_MAX);
  if (RET == g->chromo[g->len-1].x86 || LEAVE == g->chromo[g->len-1].x86) {
    gen_dump(g, stdout);
    abort();
  }

  assert(g->chromo[0].x86 < X86_COUNT);
  assert(g->chromo[1].x86 < X86_COUNT);
#endif

}

/**
 *
 */
static void gen_gen(genotype *dst, const genotype *src, const double mutate_rate)
{
  if (src) {
    /* mutate an existing genotype; by far the most common */
    *dst = *src;
    dst->len -= GEN_SUFFIX_LEN;
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
    chromo_random(dst, GEN_PREFIX_LEN, 1);
  }
  GEN_SUFFIX(dst);
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
      const genotype *src = &p->indiv[randr(0, keep-1)].geno;
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
  char hex[24],
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
    memset(h, ' ', sizeof hex - (h - hex));
    hex[23] = '\0';
    fprintf(f, "%3" PRIu32 " %s", i, hex);
    fprintf(f, x->descr, *(u32 *)&g->chromo[i].data);
    if (x->modrmlen) {
      char modbuf[16];
      fprintf(f, " %s",
        disp_modrm(g->chromo[i].modrm, x->modrm, modbuf, sizeof modbuf));
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

