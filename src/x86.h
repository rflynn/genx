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
 * instruction sets in the x86 family;
 * allows us to classify and use/disuse
 * subsets of the total instructions
 * depending on which CPU we are to run on
 */
enum iset {
  I_86,
  I_87,
  I_186,
  I_286,
  I_287,
  I_386,
  I_387,
  I_486,
  I_586,
  I_686,
  I_MMX,
  I_SSE,
  I_COUNT /* last, special */
};

/**
 * differentiate between integer and floating
 * point instructions
 */
enum iflt {
  INT = 0x1,
  FLT = 0x2
};

/**
 * differentiate between algebraic operations
 * (such as +, -, *, /, etc) and bitwise
 * operations such as <<, >>, rotate, etc.
 */
enum ialg {
  ALG = 0x1,
  BIT = 0x2 /* operation depends on base-2 or size of register */
};

enum istor {
  EAX = 1,
  EBX,
  ECX,
  EDX,
  EBP8,
  EBP_14,
  ST0,
  ST1
};

struct x86 {
  const char *descr;  /* printf-friendly format, which
                       * displays the 'rest' bytes (if any) */
  const u8 op[4],     /* instruction bytes                  */
           oplen,     /* length of instruction op, the  */
           modrmlen,
           modrm,     /* /n digit for some operations       */
           immlen;    /* number of variable bytes           */
  enum iset set;
  enum iflt flt;
  enum ialg alg;
};

enum {
  /*
   * x86 function prefix ops
   * int and float
   */
  ENTER,
  PUSH_EBP,
  MOV_ESP_EBP,
  MOV_8_EBP_EAX,  /* load eax with first parameter */
  SUB_14_ESP,     /* reserve space for local variable, 
                   * required to use random floating point
                   * datum as fp ops cannot be supplied
                   * immediate values */
  /*
   * x86 function suffix ops
   * clean up and leave function
   * int and float
   */
  ADD_14_ESP,
  LEAVE,
  POP_EBP,
  RET,

  /*
   * x86 general-purpose functions to be considered
   * for use in function body
   */

#ifdef X86_USE_INT
# define X86_FIRST ADD_IMM8
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
#endif

  LEA_8EBP_EAX,
  MOV_EAX_14EBP,

#ifdef X86_USE_FLOAT
#ifndef X86_FIRST
# define X86_FIRST LEA_8EBP_EAX
#endif
  FLD,
  MOV_IMM32_14EBP,
  FLD_14EBP,
  FILD,
#if 0
  FISTTP,
#endif
  FIST,
  FISTP,
#if 0
  F2XM1,
#endif
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
  FRNDINT,
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
#endif

#if 0
  /* instructions i have tried to add but have failed for one reason or another */
  POPCNT,
  BSWAP_EDX,
  IDIV_R32,
  JE,
  JNZ
#endif

  X86_COUNT /* last, special */
};

#endif

