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
extern struct genx_iface *Iface;

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
      *(float *)&g->chromo[i].data = randfr(Iface->test.f.min_const, Iface->test.f.max_const);
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
          *(float *)&g->chromo[i].data = randfr(Iface->test.f.min_const, Iface->test.f.max_const);
        else
#endif
          *(u32 *)&g->chromo[i].data = randr(0, Iface->test.i.max_const);
      }
#ifdef X86_USE_FLOAT
    }
#endif
  }
}

#define MAX(a,b) ((a)>(b)?(a):(b))

static void gen_mutate(genotype *g)
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
 *    GEN_PREFIX_LEN                         GEN_PREFIX_LEN + Iface->opt.chromo_max
 *          v                                               v
 * +--------------------------------------------------------+
 * | prefix |         c h r o m o s o m e s        | suffix |
 * +--------------------------------------------------------+
 *                                                 ^
 * g->len falls within the range GEN_PREFIX_LEN+(0,Iface->opt.chromo_max).
 * g->len may be modified by (-1, 0, +1) each time, as long as it
 * stays within the proper bounds.
 */

#ifdef DEBUG
  assert(g->len >= GEN_PREFIX_LEN);
  assert(g->len <= GEN_PREFIX_LEN + Iface->opt.chromo_max);
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
 * (g->len < GEN_PREFIX + Iface->opt.chromo_max) + (0, g->len - ooff)
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
 * cannot be greater than the difference between g->len and GEN_PREFIX_LEN + Iface->opt.chromo_max,
 * as it would push the suffix too far!
 */

#ifdef DEBUG
  assert(g->len - ooff < g->len);
  assert(g->len - ooff - olen < g->len);
#endif

  rlen = randr(olen - ((olen > 0) && (g->len - olen > GEN_PREFIX_LEN)),
               olen + (g->len < GEN_PREFIX_LEN + Iface->opt.chromo_max));

#ifdef DEBUG
  assert(rlen <= Iface->opt.chromo_max);
  assert(rlen <= g->len + 1);
  assert(rlen <= g->len + (g->len < GEN_PREFIX_LEN + Iface->opt.chromo_max));
  assert(g->len - ooff + (int)(rlen - olen) < GEN_PREFIX_LEN + Iface->opt.chromo_max);
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
  assert(g->len + difflen <= GEN_PREFIX_LEN + Iface->opt.chromo_max);
  assert(abs(difflen) <= (s32)g->len);
  assert(abs(difflen) <= (s32)g->len - GEN_PREFIX_LEN + 1);
#endif

  g->len += difflen;

#ifdef DEBUG
  assert(g->len > GEN_PREFIX_LEN);
  assert(g->len <= GEN_PREFIX_LEN + Iface->opt.chromo_max);

  assert(g->len <= GEN_PREFIX_LEN + Iface->opt.chromo_max);
  if (RET == g->chromo[g->len-1].x86 || LEAVE == g->chromo[g->len-1].x86) {
    gen_dump(g, stdout);
    abort();
  }

  assert(g->chromo[0].x86 < X86_COUNT);
  assert(g->chromo[1].x86 < X86_COUNT);
#endif

}

void gen_copy(genotype *dst, const genotype *src)
{
  dst->len = src->len;
  memcpy(dst->chromo, src->chromo, src->len * sizeof src->chromo[0]);
}

void genoscore_copy(genoscore *dst, const genoscore *src)
{
  dst->score = src->score;
  gen_copy(&dst->geno, &src->geno);
}

/**
 *
 */
static void gen_gen(genotype *dst, const genotype *src, const double mutate_rate)
{
  if (src) {
    /* mutate an existing genotype; by far the most common */
    gen_copy(dst, src);
    dst->len -= GEN_SUFFIX_LEN;
    do
      gen_mutate(dst);
    while (mutate_rate <= rand01());
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

#ifdef DEBUG
    const genotype *g = dst;
    assert(g->len > GEN_PREFIX_LEN);
#endif

  GEN_SUFFIX(dst);

#ifdef DEBUG
    assert(g->len > GEN_PREFIX_LEN + GEN_SUFFIX_LEN);
    assert(g->chromo[0].x86 == ENTER);
#if 0
    assert(g->chromo[1].x86 == MOV_8_EBP_EAX);
    assert(g->chromo[2].x86 == MOV_C_EBP_EBX);
    assert(g->chromo[3].x86 == MOV_10_EBP_ECX);
#endif
    assert(g->chromo[g->len - 2].x86 == LEAVE);
    assert(g->chromo[g->len - 1].x86 == RET);
#endif

}

void pop_gen(struct pop *p, const u32 keep, const genx_iface *iface)
{
  u32 i;
  if (keep > 0) {
    /*
     * if a set if [0..keep-1] "best" are set, select a random one
     * to serve as the basis for each member of the new generation
     */
    for (i = keep; i < iface->opt.pop_size; i++) {
      const genotype *src = &p->indiv[randr(0, keep-1)].geno;
      gen_gen(&p->indiv[i].geno, src, iface->opt.mutate_rate);
      GENOSCORE_SCORE(p->indiv+i) = GENOSCORE_WORST;
    }
  } else {
    /*
     * initial generation (or re-generation from scratch), do not
     * use a 'src' element
     */
    for (i = 0; i < iface->opt.pop_size; i++) {
      gen_gen(&p->indiv[i].geno, NULL, iface->opt.mutate_rate);
      GENOSCORE_SCORE(p->indiv+i) = GENOSCORE_WORST;
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
    hex[(sizeof hex) - 1] = '\0';
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
#if DEBUG
  assert(idx < g->len);
  assert(idx >= GEN_PREFIX_LEN);
#endif
  if (idx == g->len - 1)
    return 0;
  rel = idx + 1 + (rel % (g->len - idx - 2));
#if DEBUG
  assert(rel < Iface->opt.chromo_max + GEN_PREFIX_LEN + GEN_SUFFIX_LEN);
#endif
  for (i = 0; i < rel; i++)
    total += chromo_bytes(g->chromo + i);
  res = (s32)(total - off); /* calculate byte offset */
  if (res < 0) /* can't go back, no matter what */
    res = 0;
#if DEBUG
  assert(res >= 0);
#endif
  return res;
}

u32 gen_compile(genotype *g, u8 *buf, size_t buflen)
{
  u32 i,
      len = 0;
  for (i = 0; i < g->len; i++) {
#if DEBUG
    assert(g->chromo[i].x86 < X86_COUNT);
#endif
    if (X86[g->chromo[i].x86].jcc) {
      /* if relative jump, adjust the random jump destination to a valid offset */
      *(s32*)&g->chromo[i].data =
        gen_jmp_pos(g, len + chromo_bytes(g->chromo + i),
          (u32)g->chromo[i].data, i);
    }
    len = chromo_add(g->chromo + i, buf, len);
  }
  assert(i < buflen);
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

/**
 * shorter is better, given same score
 */
static int score_id_lencmp(const void *va, const void *vb)
{
  register const struct score_id *a = va,
                                 *b = vb;
  if (GENOSCORE_SCORE(a) > GENOSCORE_SCORE(b))
    return 1;
  else if (GENOSCORE_SCORE(a) < GENOSCORE_SCORE(b))
    return -1;
  else if (a->len > b->len)
    return 1;
  else if (a->len < b->len)
    return -1;
  return 0;
}

inline static void genoscore_swap(genoscore *a, genoscore *b, genoscore *tmp)
{
  genoscore_copy(tmp, a);
  genoscore_copy(a,   b);
  genoscore_copy(b,   tmp);
}

void pop_score(struct pop *p, const genx_iface *iface, genoscore *tmp)
{
  u32 w = 0;
  for (u32 i = 0; i < iface->opt.pop_size; i++) {
    score(p->indiv + i, iface, 0);
    if (GENOSCORE_NOT_WORST(p->indiv+i) || i < iface->opt.pop_keep) {
      /*
       * only count scores that are better than worst; since
       * the vast majority will be == WORST possible.
       * this greatly reduces the number of items to sort.
       * NOTE: guarentee we result in at least 'pop_keep' number
       * of unique entries
       */
      p->scores[w].score = p->indiv[i].score;
      p->scores[w].len = p->indiv[i].geno.len;
      p->scores[w].id = i;
      w++;
    }
  }
  qsort(p->scores, w, sizeof *p->scores, score_id_lencmp);
  /* copy the best pop_keep items to the front */
  for (u32 i = 0; i < iface->opt.pop_keep; i++) {
    genoscore_swap(p->indiv + i,
                   p->indiv + p->scores[i].id,
                   tmp);
  }
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

void genx_iface_dump(const genx_iface *iface)
{
  printf("genx_iface(%p):\n", (void *)iface);
  printf(" .test:\n");
  printf("  .i:\n");
  printf("   .score.......%d\n",  iface->test.i.score);
  printf("   .max_const...%lu\n", (unsigned long)iface->test.i.max_const);
  printf("   .func........%p\n",  (void *)iface->test.i.func);
  printf("   .done........%p\n",  (void *)iface->test.i.done);
  printf("   .data\n");
  printf("     .len.......%lu\n", (unsigned long)iface->test.i.data.len);
  printf("     [0]: .in { %lu, %lu, %lu, %lu } .out { %lu }\n",
                                  (unsigned long)iface->test.i.data.list[0].in[0],
                                  (unsigned long)iface->test.i.data.list[0].in[1],
                                  (unsigned long)iface->test.i.data.list[0].in[2],
                                  (unsigned long)iface->test.i.data.list[0].in[3],
                                  (unsigned long)iface->test.i.data.list[0].out);
  printf(" .opt:\n");
  printf("  .param_cnt....%lu\n", (unsigned long)iface->opt.param_cnt);
  printf("  .chromo_min...%lu\n", (unsigned long)iface->opt.chromo_min);
  printf("  .chromo_max...%lu\n", (unsigned long)iface->opt.chromo_max);
  printf("  .pop_size.....%lu\n", (unsigned long)iface->opt.pop_size);
  printf("  .pop_keep.....%lu\n", (unsigned long)iface->opt.pop_keep);
  printf("  .gen_deadend..%lu\n", (unsigned long)iface->opt.gen_deadend);
  printf("  .mutate_rate..%.3f\n", iface->opt.mutate_rate);
  printf(" .x86:\n");
  printf("  .int_ops......%d\n", iface->opt.x86.int_ops);
  printf("  .float_ops....%d\n", iface->opt.x86.float_ops);
  printf("  .algebra_ops..%d\n", iface->opt.x86.algebra_ops);
  printf("  .bit_ops......%d\n", iface->opt.x86.bit_ops);
  printf("  .random_const.%d\n", iface->opt.x86.random_const);
}


