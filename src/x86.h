/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */
/*
 * References:
 *  #1 Intel 64 and IA-32 Architectures Software Developer's Manual
 *     Volume 2A: Instruction Set Reference, A-M
 *  #2 Intel 64 and IA-32 Architectures Software Developer's Manual
 *     Volume 2N: Instruction Set Reference, N-Z
 */

#ifndef X86_H
#define X86_H

#include <stddef.h>
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
  /* begin instructions considered for general use with a function body */
//#define X86_FIRST ADD_IMM8
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
#if 0
  /* instructions i have tried to add but have failed for one reason or another */
  POPCNT,
  BSWAP_EDX,
  IDIV_R32,
  JE,
  JNZ
#endif


  /* these 4 are to generate a single random float value and
   * get it into the float stack */
  SUB_4_ESP,
  ADD_4_ESP,
  MOV_IMM32_EAX,
  MOV_EAX_4EBP,
  FLD_4EBP,


  FLD,
  FILD,
  //FISTTP,
  FIST,
  FISTP,

#define X86_FIRST F2XM1

  F2XM1,
  FPREM,
  FSQRT,
  FSINCOS,
  FSCALE,
  FSIN,
  FCOS,
  FCHS,
  FXAM,
  FLD1,
  FLDL2T,
  FLDL2E,
  FLDPI,
  FLDLG2,
  FLDLN2,
  FABS,
  FMULP,
  FADDP,

  FCOMI,
  FCOMIP,
  FUCOMI,
  FUCOMIP,

  FICOM,
  FICOMP,

  FCMOVB,
  FCMOVE,
  FCMOVBE,
  FCMOVU,
  FCMOVNB,
  FCMOVNE,
  FCMOVNBE,
  FCMOVNU,

  X86_COUNT /* last, special */
};

#endif

