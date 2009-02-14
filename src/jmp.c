
/*
 * NOTE: code is completely unused; was ripped out when project was reorganized;
 *
 * The code does work, however, because I implemented x86 JE and JNZ functions,
 * but they almost always produced never-ending functions; there are a few options
 * available to use when considering jumps:
 *
 *  - Never implement them
 *      Good: Done
 *      Bad:  Most algorithms require some kind of loop
 *
 *  - Only allow forward jumps
 *      Good: Easy
 *      Bad: Still no loops, but we get branches and no never-ending programs
 *
 *  - Implement jumps, but fork() off workers who run on a timer and kill
 *    never-ending loops:
 *      Good: ?
 *      Bad:  Waste lots of CPU (~99.9% of programs are worthless), slow, medium complexity
 *
 *  - Implement jumps, but add code to sanity-check that loops are not
 *      Good: Best solution, loops!
 *      Bad:  Will end up being very complicated; we'll need at least a barebones
 *            x86 architecture dependency implementation; and tracing all the execution
 *            paths will be complex and expensive in terms of CPU.
 *
 */

#if 0
  this goes into the gen_compile loop for jumps:

    if (JE == g->chromo[i].x86 || JNZ == g->chromo[i].x86) {
      /* if relative jump, adjust the random jump destination to a valid offset */
      g->chromo[i].data[0] = gen_jmp_pos(g, len + chromo_bytes(g->chromo + i), g->chromo[i].data[0]);
    }
#endif

#if 0 /* jmp-related */
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
 */
static s8 gen_jmp_pos(const genotype *g, const u32 off, u8 rel)
{
  u32 total = 0,
      i;
  s8 res;
  rel %= g->len - 1;  /* choose a chromosome in the genome to jump to */
  if (0 == rel)
    rel = 1;
  for (i = 0; i < rel; i++)
    total += chromo_bytes(g->chromo + i); /* calculate the total offset of that chromosome */
  res = (s8)(total - off); /* calculate byte offset */
  if (-2 == res) /* jumping to self causes never-ending loop */
    res = 0;
  return res;
}
#endif
