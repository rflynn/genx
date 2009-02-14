/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */

#ifndef X86_H
#define X86_H

#include "typ.h"
#include "gen.h"

void         x86_init(void);
u8           gen_modrm(u8 digit);
const char * disp_modrm(u8 n, const u8 modrm, char *buf, size_t len);
void         x86_dump(const u8 *x86, u32 len, FILE *f);

# define X86_NOTFOUND 0xFF
u8 x86_by_name(const char *descr);

/**
 * the different instruction sets through time;
 * at least they're backwards compatible;
 * this allows us to target different CPUs
 * NOTE: unused at the moment
 */
enum {
  CPU_88,
  CPU_86,
  CPU_286,
  CPU_386,
  CPU_486,
  CPU_586,
  CPU_686,
  CPU_MMX1,
  CPU_MMX2,
  CPU_MMX3,
  CPU_COUNT /* last, special */
};

struct x86 {
  const char *descr;  /* printf-friendly format, which
                       * displays the 'rest' bytes (if any) */
  const u8 op[4],     /* instruction bytes                  */
           oplen,     /* length of instruction op, the  */
           modrmlen,
           modrm,     /* /n digit for some operations       */
           immlen;    /* number of variable bytes           */
};

enum {
#define FUNC_PREFIX_LEN 2 /* number of chromosomes needed to initialize function */
  ENTER,
  MOV_8_EBP_EAX,
#define FUNC_SUFFIX_LEN 2 /* chromosomes always at the end */
  LEAVE,
  RET,
#define X86_FIRST ADD_IMM8
  ADD_IMM8,
  ADD_R32,
  IMUL_IMM,
  IMUL_R32,
  MOV_R32,
  XCHG_R32,
  XOR_R32,
  XOR_IMM32,
  XADD_R32,
  SHR_IMM8,
  SHL_IMM8,
  OR_R32,
  AND_R32,
  AND_IMM32,
  NEG_R32,
  NOT_R32,
  SUB_R32,
  SUB_IMM32,
  BT_R32,
  BT_IMM8,
  BSF_R32,
  BSR_R32,
  BTC_R32,
  BTC_IMM8,
  BTR_R32,
  BTR_IMM8,
  CMP_R32,
  CMP_IMM32,
  CMPXCHG_R32,
  //CRC32_R32,
  RCL_IMM8,
  RCR_IMM8,
  ROL_IMM8,
  ROR_IMM8,
  CMOVA,
  CMOVB,
  CMOVBE,
  CMOVC,
  CMOVE,
  CMOVG,
  CMOVGE,
  CMOVL,
  CMOVLE,
  CMOVNA,
  CMOVNAE,
  CMOVNB,
  CMOVNBE,
  CMOVNC,
  CMOVNE,
  CMOVNG,
  CMOVNGE,
  CMOVNL,
  CMOVNLE,
  CMOVNO,
  CMOVNP,
  CMOVNS,
  CMOVNZ,
  CMOVO,
  CMOVP,
  CMOVPE,
  CMOVPO,
  CMOVS,
  CMOVZ,
  //POPCNT,
  //BSWAP_EDX,
  //IDIV_R32,
#if 0
  ADD_EAX,
  XOR_R32,
  JE,
  JNZ
#endif
  X86_COUNT /* last, special */
};

/* populate prefix */
#define X86_GEN_PREFIX(g) do {          \
    (g)->chromo[0].x86 = ENTER;         \
    (g)->chromo[1].x86 = MOV_8_EBP_EAX; \
  } while (0)

/* populate genotype suffix */
#define X86_GEN_SUFFIX(g) do {          \
  (g)->chromo[(g)->len++].x86 = LEAVE;  \
  (g)->chromo[(g)->len++].x86 = RET;    \
  } while (0)

#endif

