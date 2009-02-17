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
  while (len--)
    fprintf(f, "%02" PRIx8 " ", *x86++);
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
  if (0 == digit) {
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
  if (0 == modrm) {
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
const struct x86 X86[X86_COUNT] = {
  /* function op */
  /* descr                        opcode              oplen,modrmlen,modrm,imm */
  { "enter"                   , { 0xc8, 0x00, 0, 0 }, 4, 0, 0, 0, I_186, 0,   0   },
  { "push    %%ebp"           , { 0x55             }, 1, 0, 0, 0, I_86,  0,   0   },
  { "mov     %%esp, %%ebp"    , { 0x89, 0xe5       }, 2, 0, 0, 0, I_86,  0,   0   },
  { "mov     0x8(%%ebp), %%eax",{ 0x8b, 0x45, 0x08 }, 3, 0, 0, 0, I_86,  0,   0   },
  { "mov     0xc(%%ebp), %%ebx",{ 0x8b, 0x5d, 0x0c }, 3, 0, 0, 0, I_86,  0,   0   },
  { "mov     0x10(%%ebp), %%ecx",{0x8b, 0x4d, 0x10 }, 3, 0, 0, 0, I_86,  0,   0   },
  { "sub     $0x14, %%esp"    , { 0x83, 0xec, 0x14 }, 3, 0, 0, 0, I_86,  0,   0   },
  /* function suffix */
  { "add     $0x14, %%esp"    , { 0x83, 0xc4, 0x14 }, 3, 0, 0, 0, I_86,  FLT, 0   },
  { "leave"                   , { 0xc9             }, 1, 0, 0, 0, I_186, 0,   0   },
  { "pop     %%ebp"           , { 0x5d             }, 1, 0, 0, 0, I_86,  0,   0   },
  { "ret"                     , { 0xc3             }, 1, 0, 0, 0, I_86,  0,   0   },

  /* function contents */

  /*
   * integer-related ops
   */
#ifdef X86_USE_INT
  { "add     0x%02" PRIx8 "," , { 0x83             }, 1, 1, 0, 1, I_86,  INT, ALG },
  { "add    "                 , { 0x01             }, 1, 1, 0, 0, I_86,  INT, ALG },
  { "imul    0x%02" PRIx8 "," , { 0x6b             }, 1, 1, 0, 1, I_86,  INT, ALG },
  { "imul   "                 , { 0x0f, 0xaf       }, 2, 1, 0, 0, I_86,  INT, ALG },
  { "mov    "                 , { 0x8b             }, 1, 1, 0, 0, I_86,  INT, ALG },
  { "xchg   "                 , { 0x87             }, 1, 1, 0, 0, I_86,  INT, ALG },
  { "xor    "                 , { 0x33             }, 1, 1, 0, 0, I_86,  INT, BIT },
  { "xor    "                 , { 0x81             }, 1, 1, 6, 4, I_86,  INT, BIT },
  { "xadd   "                 , { 0x0f, 0xc1       }, 2, 1, 0, 0, I_486, INT, ALG },
  { "shr     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 5, 1, I_86,  INT, ALG },
  { "shl     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 4, 1, I_86,  INT, ALG },
  { "or     "                 , { 0x0b             }, 1, 1, 0, 0, I_86,  INT, ALG },
  { "and    "                 , { 0x23             }, 1, 1, 0, 0, I_86,  INT, ALG },
  { "and     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 4, 4, I_86,  INT, ALG },
  { "neg    "                 , { 0xf7             }, 1, 1, 3, 0, I_86,  INT, ALG },
  { "not    "                 , { 0xf7             }, 1, 1, 2, 0, I_86,  INT, ALG },
  { "sub    "                 , { 0x29             }, 1, 1, 0, 0, I_86,  INT, ALG },
  { "sub     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 5, 4, I_86,  INT, ALG },
  { "bt     "                 , { 0x0f, 0xa3       }, 2, 1, 0, 0, I_386, INT, ALG },
  { "bt      0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 4, 1, I_386, INT, ALG },
  { "bsf    "                 , { 0x0f, 0xbc       }, 2, 1, 0, 0, I_386, INT, BIT },
  { "bsr    "                 , { 0x0f, 0xbd       }, 2, 1, 0, 0, I_386, INT, BIT },
  { "btc    "                 , { 0x0f, 0xbb       }, 2, 1, 0, 0, I_386, INT, BIT },
  { "btc     0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 7, 1, I_386, INT, BIT },
  { "btr    "                 , { 0x0f, 0xb3       }, 2, 1, 0, 0, I_386, INT, BIT },
  { "btr     0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 6, 1, I_386, INT, ALG },
  { "cmp    "                 , { 0x39             }, 1, 1, 0, 0, I_386, INT, ALG },
  { "cmp     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 7, 4, I_386, INT, ALG },
  { "cmpxchg"                 , { 0x0f, 0xb1       }, 2, 1, 0, 0, I_486, INT, ALG },
  { "rcl     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 2, 1, I_86,  INT, BIT },
  { "rcr     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 3, 1, I_86,  INT, BIT },
  { "rol     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 0, 1, I_86,  INT, BIT },
  { "ror     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 1, 1, I_86,  INT, BIT },
  { "cmova  "                 , { 0x0f, 0x47       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovb  "                 , { 0x0f, 0x42       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovbe "                 , { 0x0f, 0x46       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovc  "                 , { 0x0f, 0x42       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmove  "                 , { 0x0f, 0x44       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovg  "                 , { 0x0f, 0x4f       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovge "                 , { 0x0f, 0x4d       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovl  "                 , { 0x0f, 0x4c       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovle "                 , { 0x0f, 0x4e       }, 2, 1, 0, 0, I_686, INT, ALG },
#if 0
  { "cmovna "                 , { 0x0f, 0x46       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnae"                 , { 0x0f, 0x42       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnb "                 , { 0x0f, 0x43       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnbe"                 , { 0x0f, 0x47       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnc "                 , { 0x0f, 0x43       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovne "                 , { 0x0f, 0x45       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovng "                 , { 0x0f, 0x4e       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnge"                 , { 0x0f, 0x4c       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnl "                 , { 0x0f, 0x4d       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnle"                 , { 0x0f, 0x4f       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovno "                 , { 0x0f, 0x41       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnp "                 , { 0x0f, 0x4b       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovns "                 , { 0x0f, 0x49       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovnz "                 , { 0x0f, 0x45       }, 2, 1, 0, 0, I_686, INT, ALG },
#endif
  { "cmovo  "                 , { 0x0f, 0x40       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovp  "                 , { 0x0f, 0x4a       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovpe "                 , { 0x0f, 0x4a       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovpo "                 , { 0x0f, 0x4b       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovs  "                 , { 0x0f, 0x48       }, 2, 1, 0, 0, I_686, INT, ALG },
  { "cmovz  "                 , { 0x0f, 0x44       }, 2, 1, 0, 0, I_686, INT, ALG },
#endif

  { "lea     0x8(%%ebp), %%eax" ,{ 0x8d, 0x45, 0x08}, 3, 0, 0, 0, I_86,  0,   0   },
  { "mov     %%eax, -0x14(%%ebp)",{0x8b, 0x45, 0xce}, 3, 0, 0, 0, I_86,  0,   0   },

/*
 * x86 floating point operations
 */

#ifdef X86_USE_FLOAT
  { "fld     0x8(%%ebp)"      , { 0xd9, 0x45, 0x08 }, 3, 0, 0, 0, I_87,  FLT, 0   },
  { "mov     $0x%08" PRIx32 ", -0x14(%%ebp)",
                                { 0xc7, 0x45, 0xec }, 3, 0, 0, 4, I_86,  FLT, 0   },
  { "fld     -0x14(%%ebp)"    , { 0xd9, 0x45, 0xec }, 3, 0, 0, 0, I_87,  FLT, 0   },
  { "fild    0x8(%%ebp)"      , { 0xdb, 0x45, 0x08 }, 3, 0, 0, 0, I_87,  FLT, 0   },
#if 0
  /*
   * doesn't work on my Pentium 3 at home
   * TODO: add ability to generate opcodes by target machine
   */
  { "fisttp  0x8(%%ebp)"      , { 0xdb, 0x4d, 0x08 }, 3, 0, 0, 0, I_SSE, FLT, ALG },
#endif
  { "fist    0x8(%%ebp)"      , { 0xdb, 0x55, 0x08 }, 3, 0, 0, 0, I_87,  FLT, 0   },
  { "fistp   0x8(%%ebp)"      , { 0xdb, 0x5d, 0x08 }, 3, 0, 0, 0, I_87,  FLT, 0   },
#if 0
  /* this instruction is expensive in terms of cycles */
  { "f2xm1"                   , { 0xd9, 0xf0       }, 2, 0, 0, 0, I_87,  FLT, ALG },
#endif
  { "fprem"                   , { 0xd9, 0xf8       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fsqrt"                   , { 0xd9, 0xfa       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fsincos"                 , { 0xd9, 0xfb       }, 2, 0, 0, 0, I_387, FLT, ALG },
  { "fscale"                  , { 0xd9, 0xfd       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fsin"                    , { 0xd9, 0xfe       }, 2, 0, 0, 0, I_387, FLT, ALG },
  { "fcos"                    , { 0xd9, 0xff       }, 2, 0, 0, 0, I_387, FLT, ALG },
  { "fchs"                    , { 0xd9, 0xe0       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fxam"                    , { 0xd9, 0xe5       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fld1"                    , { 0xd9, 0xe8       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fldl2t"                  , { 0xd9, 0xe9       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fldl2e"                  , { 0xd9, 0xea       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fldpi"                   , { 0xd9, 0xeb       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fldlg2"                  , { 0xd9, 0xec       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fldln2"                  , { 0xd9, 0xed       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fabs"                    , { 0xd9, 0xe1       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fmulp"                   , { 0xde, 0xc9       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "faddp"                   , { 0xde, 0xc1       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "frndint"                 , { 0xd9, 0xfc       }, 2, 0, 0, 0, I_87,  FLT, ALG },
  { "fcomi   %%st,%%st(1)"    , { 0xdb, 0xf1       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fcomip  %%st,%%st(1)"    , { 0xdf, 0xf1       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fucomi  %%st,%%st(1)"    , { 0xdb, 0xe9       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fucomip %%st,%%st(1)"    , { 0xdf, 0xe9       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "ficom   %%st(0),0x8(%%ebp)",{0xda, 0x55, 0x08 }, 3, 0, 0, 0, I_87,  FLT, ALG },
  { "ficomp  %%st(0),0x8(%%ebp)",{0xda, 0x5d, 0x08 }, 3, 0, 0, 0, I_87,  FLT, ALG },
  { "fcmovb  %%st(0),%%st(1)" , { 0xda, 0xc1       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fcmove  %%st(0),%%st(1)" , { 0xda, 0xc9       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fcmovbe %%st(0),%%st(1)" , { 0xda, 0xd1       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fcmovu  %%st(0),%%st(1)" , { 0xda, 0xd9       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fcmovnb %%st(0),%%st(1)" , { 0xdb, 0xc1       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fcmovne %%st(0),%%st(1)" , { 0xdb, 0xc1       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fcmovnbe %%st(0),%%st(1)", { 0xdb, 0xd1       }, 2, 0, 0, 0, I_686, FLT, ALG },
  { "fcmovnu %%st(0),%%st(1)" , { 0xdb, 0xd9       }, 2, 0, 0, 0, I_686, FLT, ALG },
#endif

};

/**
 * find an x86 instruction by name
 */
u8 x86_by_name(const char *descr)
{
  unsigned i;
  u8 off = X86_NOTFOUND;
  size_t dlen = strlen(descr);
  for (i = 0; i < sizeof X86 / sizeof X86[0]; i++) {
    if (0 == strncmp(descr, X86[i].descr, dlen)) {
      off = (u8)i;
      break;
    }
  }
  return off;
}

void x86_init(void)
{
  /* double-check instruction enum and table */
  printf("X86_COUNT=%u (sizeof X86 / sizeof X86[0])=%u\n",
    X86_COUNT, sizeof X86 / sizeof X86[0]);
#if 0
  assert(0 == strncmp("or",      X86[OR_R32]     .descr, 2));
  assert(0 == strncmp("cmpxchg", X86[CMPXCHG_R32].descr, 7));
  assert(0 == strncmp("cmova",   X86[CMOVA]      .descr, 5));
  assert(0 == strncmp("cmovge",  X86[CMOVGE]     .descr, 6));
  assert(0 == strncmp("cmovne",  X86[CMOVNE]     .descr, 6));
  assert(0 == strncmp("cmovz",   X86[CMOVZ]      .descr, 5));
#endif
}

