/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */
/*
 * References: see x86.h
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "typ.h"
#include "rnd.h"
#include "x86.h"

void x86_dump(const u8 *x86, u32 len, FILE *f)
{
  assert(len < 1024);
  while (len--)
    fprintf(f, "<%02" PRIx8 "> ", *x86++);
  fputc('\n', f);
}

/**
 * modr/m byte specifies src and dst registers
 * @ref: #1 S 2-6 Table 2-1
 */
u8 gen_modrm(u8 digit)
{
  static const u8 x[] = {
          /* dst <- src */
    0xc0, /* eax <- eax */
    0xd8, /* ebx <- eax */
    0xc8, /* ecx <- eax */
    0xd0, /* edx <- eax */

    0xc3, /* eax <- ebx */
    0xdb, /* ebx <- ebx */
    0xcb, /* ecx <- ebx */
    0xd3, /* edx <- ebx */

    0xc1, /* eax <- ecx */
    0xd9, /* ebx <- ecx */
    0xc9, /* ecx <- ecx */
    0xd1, /* edx <- ecx */

    0xc2, /* eax <- edx */
    0xda, /* ebx <- edx */
    0xca, /* ecx <- edx */
    0xd2  /* edx <- edx */
  };
  u8 modrm;
  if (R == digit) {
    /*
     * NOTE: would be faster to calculate byte directly instead
     * of using memory
     */
    modrm = x[rnd32() & 0xF];
  } else {
    /* 
     * NOTE: only includes e[abcd]x can't modify esp or ebp; but this
     * solution makes it impossible to utilize e[sd]i
     */
    modrm = (0xc0 | (digit << 3)) + (rnd32() & 0x3);
  }
  return modrm;
}

const const char * disp_modrm(u8 n, const u8 modrm, char *buf, size_t len)
{
  static const char reg[8][4] = {
    "eax",
    "ecx",
    "edx",
    "ebx",
    "esp",
    "ebp",
    "esi",
    "edi"
  };
  if (R == modrm) {
    n -= 0xc0;
    snprintf(buf, len, "%%%s, %%%s", reg[n >> 3], reg[n & 7]);
  } else {
    snprintf(buf, len, "%%%s", reg[n & 7]);
  }
  return buf;
}

/*
 * set of x86 instruction templates
 * for each instruction we support we have enough information to
 * generate a random but valid invocation, and the ability to describe
 * the operation in asm (at&t syntax atm, though i prefer intel)
 * @ref #1
 * @ref #2
 */
const struct x86 X86_[X86_CNT] = {
  /* function op */
  /* descr                        opcode              oplen,modrmlen,modrm,imm */
  { "enter"                   , { 0xc8, 0x00, 0, 0 }, 4, 0, R, 0, 0, I_186, 0,   0   },
  { "push    %%ebp"           , { 0x55             }, 1, 0, R, 0, 0, I_86,  0,   0   },
  { "mov     %%esp, %%ebp"    , { 0x89, 0xe5       }, 2, 0, R, 0, 0, I_86,  0,   0   },
  { "mov     0x8(%%ebp), %%eax",{ 0x8b, 0x45, 0x08 }, 3, 0, R, 0, 0, I_86,  0,   0   },
  { "mov     0xc(%%ebp), %%ebx",{ 0x8b, 0x5d, 0x0c }, 3, 0, R, 0, 0, I_86,  0,   0   },
  { "mov     0x10(%%ebp), %%ecx",{0x8b, 0x4d, 0x10 }, 3, 0, R, 0, 0, I_86,  0,   0   },
  { "sub     $0x14, %%esp"    , { 0x83, 0xec, 0x14 }, 3, 0, R, 0, 0, I_86,  0,   0   },
  /* function suffix */
  { "add     $0x14, %%esp"    , { 0x83, 0xc4, 0x14 }, 3, 0, R, 0, 0, I_86,  FLT, 0   },
  { "leave"                   , { 0xc9             }, 1, 0, R, 0, 0, I_186, 0,   0   },
  { "pop     %%ebp"           , { 0x5d             }, 1, 0, R, 0, 0, I_86,  0,   0   },
  { "ret"                     , { 0xc3             }, 1, 0, R, 0, 0, I_86,  0,   0   },

  /* function contents */

  /*
   * jump instructions
   * NOTE: many opcodes have more than one associated mnuemonic, we
   *       strip all that shit out to increase signal/noise
   */
  { "ja      0x%08" PRIx32    , { 0x0f, 0x87       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jae     0x%08" PRIx32    , { 0x0f, 0x83       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jb      0x%08" PRIx32    , { 0x0f, 0x82       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jbe     0x%08" PRIx32    , { 0x0f, 0x86       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "je      0x%08" PRIx32    , { 0x0f, 0x84       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jg      0x%08" PRIx32    , { 0x0f, 0x8f       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jge     0x%08" PRIx32    , { 0x0f, 0x8d       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jl      0x%08" PRIx32    , { 0x0f, 0x8c       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jle     0x%08" PRIx32    , { 0x0f, 0x8e       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jne     0x%08" PRIx32    , { 0x0f, 0x85       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jno     0x%08" PRIx32    , { 0x0f, 0x81       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jnp     0x%08" PRIx32    , { 0x0f, 0x8b       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jns     0x%08" PRIx32    , { 0x0f, 0x89       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jo      0x%08" PRIx32    , { 0x0f, 0x80       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "jp      0x%08" PRIx32    , { 0x0f, 0x8a       }, 2, 0, R, 4, 1, I_86,  0,   0   },
  { "js      0x%08" PRIx32    , { 0x0f, 0x88       }, 2, 0, R, 4, 1, I_86,  0,   0   },

  /*
   * integer-related ops
   */
  { "add     0x%02" PRIx8 "," , { 0x83             }, 1, 1, R, 1, 0, I_86,  INT, ALG },
  { "add    "                 , { 0x01             }, 1, 1, R, 0, 0, I_86,  INT, ALG },
  { "imul    0x%02" PRIx8 "," , { 0x6b             }, 1, 1, R, 1, 0, I_86,  INT, ALG },
  { "imul   "                 , { 0x0f, 0xaf       }, 2, 1, R, 0, 0, I_86,  INT, ALG },
  { "mov    "                 , { 0x8b             }, 1, 1, R, 0, 0, I_86,  INT, ALG },
  { "xchg   "                 , { 0x87             }, 1, 1, R, 0, 0, I_86,  INT, 0   },
  { "xor    "                 , { 0x33             }, 1, 1, R, 0, 0, I_86,  INT, BIT },
  { "xor     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 6, 4, 0, I_86,  INT, BIT },
  { "xadd   "                 , { 0x0f, 0xc1       }, 2, 1, R, 0, 0, I_486, INT, ALG },
  { "shr     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 5, 1, 0, I_86,  INT, BIT },
  { "shl     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 4, 1, 0, I_86,  INT, BIT },
  { "or     "                 , { 0x0b             }, 1, 1, R, 0, 0, I_86,  INT, ALG },
  { "and    "                 , { 0x23             }, 1, 1, R, 0, 0, I_86,  INT, ALG },
  { "and     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 4, 4, 0, I_86,  INT, ALG },
  { "neg    "                 , { 0xf7             }, 1, 1, 3, 0, 0, I_86,  INT, ALG },
  { "not    "                 , { 0xf7             }, 1, 1, 2, 0, 0, I_86,  INT, ALG },
  { "sub    "                 , { 0x29             }, 1, 1, R, 0, 0, I_86,  INT, ALG },
  { "sub     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 5, 4, 0, I_86,  INT, ALG },
  { "bt     "                 , { 0x0f, 0xa3       }, 2, 1, R, 0, 0, I_386, INT, ALG },
  { "bt      0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 4, 1, 0, I_386, INT, ALG },
  { "bsf    "                 , { 0x0f, 0xbc       }, 2, 1, R, 0, 0, I_386, INT, BIT },
  { "bsr    "                 , { 0x0f, 0xbd       }, 2, 1, R, 0, 0, I_386, INT, BIT },
  { "btc    "                 , { 0x0f, 0xbb       }, 2, 1, R, 0, 0, I_386, INT, BIT },
  { "btc     0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 7, 1, 0, I_386, INT, BIT },
  { "btr    "                 , { 0x0f, 0xb3       }, 2, 1, R, 0, 0, I_386, INT, BIT },
  { "btr     0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 6, 1, 0, I_386, INT, ALG },
  { "cmp    "                 , { 0x39             }, 1, 1, R, 0, 0, I_386, INT, ALG },
  { "cmp     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 7, 4, 0, I_386, INT, ALG },
  { "cmpxchg"                 , { 0x0f, 0xb1       }, 2, 1, R, 0, 0, I_486, INT, 0   },
  { "rcl     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 2, 1, 0, I_86,  INT, BIT },
  { "rcr     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 3, 1, 0, I_86,  INT, BIT },
  { "rol     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, R, 1, 0, I_86,  INT, BIT },
  { "ror     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 1, 1, 0, I_86,  INT, BIT },
  { "cmova  "                 , { 0x0f, 0x47       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovb  "                 , { 0x0f, 0x42       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovbe "                 , { 0x0f, 0x46       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovc  "                 , { 0x0f, 0x42       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmove  "                 , { 0x0f, 0x44       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovg  "                 , { 0x0f, 0x4f       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovge "                 , { 0x0f, 0x4d       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovl  "                 , { 0x0f, 0x4c       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovle "                 , { 0x0f, 0x4e       }, 2, 1, R, 0, 0, I_686, INT, 0   },
#if 0
  { "cmovna "                 , { 0x0f, 0x46       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnae"                 , { 0x0f, 0x42       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnb "                 , { 0x0f, 0x43       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnbe"                 , { 0x0f, 0x47       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnc "                 , { 0x0f, 0x43       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovne "                 , { 0x0f, 0x45       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovng "                 , { 0x0f, 0x4e       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnge"                 , { 0x0f, 0x4c       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnl "                 , { 0x0f, 0x4d       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnle"                 , { 0x0f, 0x4f       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovno "                 , { 0x0f, 0x41       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnp "                 , { 0x0f, 0x4b       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovns "                 , { 0x0f, 0x49       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovnz "                 , { 0x0f, 0x45       }, 2, 1, R, 0, 0, I_686, INT, 0   },
#endif
  { "cmovo  "                 , { 0x0f, 0x40       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovp  "                 , { 0x0f, 0x4a       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovpe "                 , { 0x0f, 0x4a       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovpo "                 , { 0x0f, 0x4b       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovs  "                 , { 0x0f, 0x48       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "cmovz  "                 , { 0x0f, 0x44       }, 2, 1, R, 0, 0, I_686, INT, 0   },
  { "inc    "                 , { 0xff             }, 1, 1, 0, 0, 0, I_86,  INT, 0   },
  { "dec    "                 , { 0xff             }, 1, 1, 1, 0, 0, I_86,  INT, 0   },

  { "lea     0x8(%%ebp), %%eax" ,{ 0x8d, 0x45, 0x08}, 3, 0, R, 0, 0, I_86,  0,   0   },

  { "seta   "                   ,{ 0x0f, 0x97      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setae  "                   ,{ 0x0f, 0x93      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setb   "                   ,{ 0x0f, 0x92      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setbe  "                   ,{ 0x0f, 0x96      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "sete   "                   ,{ 0x0f, 0x94      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setg   "                   ,{ 0x0f, 0x9f      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setge  "                   ,{ 0x0f, 0x9d      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setl   "                   ,{ 0x0f, 0x9c      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setle  "                   ,{ 0x0f, 0x9e      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setne  "                   ,{ 0x0f, 0x95      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setns  "                   ,{ 0x0f, 0x99      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "seto   "                   ,{ 0x0f, 0x90      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setp   "                   ,{ 0x0f, 0x9a      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "setpo  "                   ,{ 0x0f, 0x9b      }, 2, 1, R, 0, 0, I_386, 0,   0   },
  { "sets   "                   ,{ 0x0f, 0x98      }, 2, 1, R, 0, 0, I_386, 0,   0   },

/*
 * x86 floating point operations
 */

  { "mov     %%eax, -0x14(%%ebp)",{0x8b, 0x45, 0xce}, 3, 0, R, 0, 0, I_86,  0,   0   },
  { "fld     0x8(%%ebp)"      , { 0xd9, 0x45, 0x08 }, 3, 0, R, 0, 0, I_87,  FLT, 0   },
  { "mov     $0x%08" PRIx32 ", -0x14(%%ebp)",
                                { 0xc7, 0x45, 0xec }, 3, 0, R, 4, 0, I_86,  FLT, 0   },
  { "fld     -0x14(%%ebp)"    , { 0xd9, 0x45, 0xec }, 3, 0, R, 0, 0, I_87,  FLT, 0   },
  { "fild    0x8(%%ebp)"      , { 0xdb, 0x45, 0x08 }, 3, 0, R, 0, 0, I_87,  FLT, 0   },
#if 0
  /*
   * doesn't work on my Pentium 3 at home
   * TODO: add ability to generate opcodes by target machine
   */
  { "fisttp  0x8(%%ebp)"      , { 0xdb, 0x4d, 0x08 }, 3, 0, R, 0, 0, I_SSE, FLT, ALG },
#endif
  { "fist    0x8(%%ebp)"      , { 0xdb, 0x55, 0x08 }, 3, 0, R, 0, 0, I_87,  FLT, 0   },
  { "fistp   0x8(%%ebp)"      , { 0xdb, 0x5d, 0x08 }, 3, 0, R, 0, 0, I_87,  FLT, 0   },
#if 0
  /* this instruction is expensive in terms of cycles */
  { "f2xm1"                   , { 0xd9, 0xf0       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
#endif
  { "fprem"                   , { 0xd9, 0xf8       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fsqrt"                   , { 0xd9, 0xfa       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fsincos"                 , { 0xd9, 0xfb       }, 2, 0, R, 0, 0, I_387, FLT, ALG },
  { "fscale"                  , { 0xd9, 0xfd       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fsin"                    , { 0xd9, 0xfe       }, 2, 0, R, 0, 0, I_387, FLT, ALG },
  { "fcos"                    , { 0xd9, 0xff       }, 2, 0, R, 0, 0, I_387, FLT, ALG },
  { "fchs"                    , { 0xd9, 0xe0       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fxam"                    , { 0xd9, 0xe5       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fld1"                    , { 0xd9, 0xe8       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fldl2t"                  , { 0xd9, 0xe9       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fldl2e"                  , { 0xd9, 0xea       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fldpi"                   , { 0xd9, 0xeb       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fldlg2"                  , { 0xd9, 0xec       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fldln2"                  , { 0xd9, 0xed       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fabs"                    , { 0xd9, 0xe1       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fmulp"                   , { 0xde, 0xc9       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "faddp"                   , { 0xde, 0xc1       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "frndint"                 , { 0xd9, 0xfc       }, 2, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fcomi   %%st,%%st(1)"    , { 0xdb, 0xf1       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fcomip  %%st,%%st(1)"    , { 0xdf, 0xf1       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fucomi  %%st,%%st(1)"    , { 0xdb, 0xe9       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fucomip %%st,%%st(1)"    , { 0xdf, 0xe9       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "ficom   %%st(0),0x8(%%ebp)",{0xda, 0x55, 0x08 }, 3, 0, R, 0, 0, I_87,  FLT, ALG },
  { "ficomp  %%st(0),0x8(%%ebp)",{0xda, 0x5d, 0x08 }, 3, 0, R, 0, 0, I_87,  FLT, ALG },
  { "fcmovb  %%st(0),%%st(1)" , { 0xda, 0xc1       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fcmove  %%st(0),%%st(1)" , { 0xda, 0xc9       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fcmovbe %%st(0),%%st(1)" , { 0xda, 0xd1       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fcmovu  %%st(0),%%st(1)" , { 0xda, 0xd9       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fcmovnb %%st(0),%%st(1)" , { 0xdb, 0xc1       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fcmovne %%st(0),%%st(1)" , { 0xdb, 0xc1       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fcmovnbe %%st(0),%%st(1)", { 0xdb, 0xd1       }, 2, 0, R, 0, 0, I_686, FLT, ALG },
  { "fcmovnu %%st(0),%%st(1)" , { 0xdb, 0xd9       }, 2, 0, R, 0, 0, I_686, FLT, ALG },

};

/**
 * find an x86 instruction by name
 */
u8 x86_by_name(const char *descr)
{
  unsigned i;
  u8 off = X86_NOTFOUND;
  size_t dlen = strlen(descr);
  for (i = 0; i < sizeof X86_ / sizeof X86_[0]; i++) {
    if (0 == strncmp(descr, X86_[i].descr, dlen)) {
      off = (u8)i;
      break;
    }
  }
  return off;
}

struct x86 X86[X86_CNT];
u32 X86_COUNT = 0;

/**
 * construct a table of instructions from X86_ that match the options in
 * iface->opt
 */
static void build_instr(const genx_iface *iface)
{
  u32 i;
  /*
   * unconditionally include all special instructions
   * that appear before X86_FIRST
   */
  for (i = 0; i < X86_FIRST; i++) {
    X86[i] = X86_[i];
  }
  X86_COUNT = i;
  /*
   * filter instructions by the flags
   */
  for (; i < X86_CNT; i++) {
    if (
      (INT == X86_[i].flt && !iface->opt.x86.int_ops) ||
      (FLT == X86_[i].flt && !iface->opt.x86.float_ops) ||
      (ALG == X86_[i].alg && !iface->opt.x86.algebra_ops) ||
      (BIT == X86_[i].alg && !iface->opt.x86.bit_ops) ||
      (X86_[i].immlen     && !iface->opt.x86.random_const)
    ) {
      /* doesn't match, skip */
      continue;
    }
    X86[X86_COUNT++] = X86_[i];
  }
}

void x86_init(const genx_iface *iface)
{
  /* double-check instruction enum and table */
  printf("X86_CNT=%u (sizeof X86 / sizeof X86[0])=%lu\n",
    X86_CNT, (unsigned long)(sizeof X86 / sizeof X86[0]));
#if 0
  assert(0 == strncmp("or",      X86[OR_R32]     .descr, 2));
  assert(0 == strncmp("cmpxchg", X86[CMPXCHG_R32].descr, 7));
  assert(0 == strncmp("cmova",   X86[CMOVA]      .descr, 5));
  assert(0 == strncmp("cmovge",  X86[CMOVGE]     .descr, 6));
  assert(0 == strncmp("cmovne",  X86[CMOVNE]     .descr, 6));
  assert(0 == strncmp("cmovz",   X86[CMOVZ]      .descr, 5));
#endif
  build_instr(iface);
  printf("X86_COUNT=%u\n", X86_COUNT);
}

