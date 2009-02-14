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
  /* descr                        opcode                    oplen,modrmlen,modrm,imm */
  { "enter"                   , { 0xc8, 0x00, 0, 0 }, 4, 0, 0, 0 }, /*       ENTER   imm16, 0           */
  { "mov     0x8(%%ebp),  %%eax",{0x8b, 0x45, 8    }, 3, 0, 0, 0 }, /* /r    MOV     r32,   r/m32       */
  /* function suffix */
  { "leave"                   , { 0xc9             }, 1, 0, 0, 0 }, /*       LEAVE                      */
  { "ret"                     , { 0xc3             }, 1, 0, 0, 0 }, /*       RET                        */
  /* function contents */
  { "add     0x%02" PRIx8 "," , { 0x83             }, 1, 1, 0, 1 }, /* /r    ADD     r/m32, imm8        */
  { "add    "                 , { 0x01             }, 1, 1, 0, 0 }, /* /r    ADD     r/m32, r32         */
  { "imul    0x%02" PRIx8 "," , { 0x6b             }, 1, 1, 0, 1 }, /* /r ib IMUL    r32,   r/m32, imm8 */
  { "imul   "                 , { 0x0f, 0xaf       }, 2, 1, 0, 0 }, /* /r    IMUL    r32,   r/m32       */
  { "mov    "                 , { 0x8b             }, 1, 1, 0, 0 }, /* /r    MOV     r32,   r/m32       */
  { "xchg   "                 , { 0x87             }, 1, 1, 0, 0 }, /* /r    XCHG    r32,   r/m32       */
  { "xor    "                 , { 0x33             }, 1, 1, 0, 0 }, /* /r    XOR     r32,   r/m32       */
  { "xor    "                 , { 0x81             }, 1, 1, 6, 4 }, /* /6 id XOR     r/m32, imm32       */
  { "xadd   "                 , { 0x0f, 0xc1       }, 2, 1, 0, 0 }, /* /r    XADD    r/m32, r32         */
  { "shr     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 5, 1 }, /* /5 ib SHR     r/m32, imm8        */
  { "shl     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 4, 1 }, /* /4 ib SHL     r/m32, imm8        */
  { "or     "                 , { 0x0b             }, 1, 1, 0, 0 }, /* /r    OR      r32,   r/m32       */
  { "and    "                 , { 0x23             }, 1, 1, 0, 0 }, /* /r    AND     r32,   r/m32       */
  { "and     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 4, 4 }, /* /4 id AND     r/m32, imm32       */
  { "neg    "                 , { 0xf7             }, 1, 1, 3, 0 }, /* /3    NEG     r/m32              */
  { "not    "                 , { 0xf7             }, 1, 1, 2, 0 }, /* /2    NOT     r/m32              */
  { "sub    "                 , { 0x29             }, 1, 1, 0, 0 }, /* /r    SUB     r/m32, r32         */
  { "sub     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 5, 4 }, /* /5 id SUB     r/m32, imm32       */
  { "bt     "                 , { 0x0f, 0xa3       }, 2, 1, 0, 0 }, /*       BT      r/m32, r32         */
  { "bt      0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 4, 1 }, /* /4 ib BT      r/m32, imm8        */
  { "bsf    "                 , { 0x0f, 0xbc       }, 2, 1, 0, 0 }, /* /r    BSF     r32,   r/m32       */
  { "bsr    "                 , { 0x0f, 0xbd       }, 2, 1, 0, 0 }, /* /r    BSR     r32,   r/m32       */
  { "btc    "                 , { 0x0f, 0xbb       }, 2, 1, 0, 0 }, /*       BTC     r/m32, r32         */
  { "btc     0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 7, 1 }, /* /7 ib BTC     r/m32, imm8        */
  { "btr    "                 , { 0x0f, 0xb3       }, 2, 1, 0, 0 }, /*       BTR     r/m32, r32         */
  { "btr     0x%02" PRIx8 "," , { 0x0f, 0xba       }, 2, 1, 6, 1 }, /* /6 ib BTR     r/m32, imm8        */
  { "cmp    "                 , { 0x39             }, 1, 1, 0, 0 }, /* /r    CMP     r/m32, r32         */
  { "cmp     0x%08" PRIx32 ",", { 0x81             }, 1, 1, 7, 4 }, /* /7 id CMP     r/m32, imm32       */
  { "cmpxchg"                 , { 0x0f, 0xb1       }, 2, 1, 0, 0 }, /* /r    CMPXCHG r/m32, r32         */
  { "rcl     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 2, 1 }, /* /2 ib RCL     r/m32, imm8        */
  { "rcr     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 3, 1 }, /* /3 ib RCR     r/m32, imm8        */
  { "rol     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 0, 1 }, /* /0 ib ROL     r/m32, imm8        */
  { "ror     0x%02" PRIx8 "," , { 0xc1             }, 1, 1, 1, 1 }, /* /1 ib ROR     r/m32, imm8        */
  { "cmova  "                 , { 0x0f, 0x47       }, 2, 1, 0, 0 }, /* /r    CMOVA   r32,   r/m32       */
  { "cmovb  "                 , { 0x0f, 0x42       }, 2, 1, 0, 0 }, /* /r    CMOVB   r32,   r/m32       */
  { "cmovbe "                 , { 0x0f, 0x46       }, 2, 1, 0, 0 }, /* /r    CMOVBE  r32,   r/m32       */
  { "cmovc  "                 , { 0x0f, 0x42       }, 2, 1, 0, 0 }, /* /r    CMOVC   r32,   r/m32       */
  { "cmove  "                 , { 0x0f, 0x44       }, 2, 1, 0, 0 }, /* /r    CMOVE   r32,   r/m32       */
  { "cmovg  "                 , { 0x0f, 0x4f       }, 2, 1, 0, 0 }, /* /r    CMOVG   r32,   r/m32       */
  { "cmovge "                 , { 0x0f, 0x4d       }, 2, 1, 0, 0 }, /* /r    CMOVGE  r32,   r/m32       */
  { "cmovl  "                 , { 0x0f, 0x4c       }, 2, 1, 0, 0 }, /* /r    CMOVL   r32,   r/m32       */
  { "cmovle "                 , { 0x0f, 0x4e       }, 2, 1, 0, 0 }, /* /r    CMOVLE  r32,   r/m32       */
  { "cmovna "                 , { 0x0f, 0x46       }, 2, 1, 0, 0 }, /* /r    CMOVNA  r32,   r/m32       */
  { "cmovnae"                 , { 0x0f, 0x42       }, 2, 1, 0, 0 }, /* /r    CMOVNAE r32,   r/m32       */
  { "cmovnb "                 , { 0x0f, 0x43       }, 2, 1, 0, 0 }, /* /r    CMOVNB  r32,   r/m32       */
  { "cmovnbe"                 , { 0x0f, 0x47       }, 2, 1, 0, 0 }, /* /r    CMOVNBE r32,   r/m32       */
  { "cmovnc "                 , { 0x0f, 0x43       }, 2, 1, 0, 0 }, /* /r    CMOVNC  r32,   r/m32       */
  { "cmovne "                 , { 0x0f, 0x45       }, 2, 1, 0, 0 }, /* /r    CMOVNE  r32,   r/m32       */
  { "cmovng "                 , { 0x0f, 0x4e       }, 2, 1, 0, 0 }, /* /r    CMOVNG  r32,   r/m32       */
  { "cmovnge"                 , { 0x0f, 0x4c       }, 2, 1, 0, 0 }, /* /r    CMOVNGE r32,   r/m32       */
  { "cmovnl "                 , { 0x0f, 0x4d       }, 2, 1, 0, 0 }, /* /r    CMOVNL  r32,   r/m32       */
  { "cmovnle"                 , { 0x0f, 0x4f       }, 2, 1, 0, 0 }, /* /r    CMOVNLE r32,   r/m32       */
  { "cmovno "                 , { 0x0f, 0x41       }, 2, 1, 0, 0 }, /* /r    CMOVNO  r32,   r/m32       */
  { "cmovnp "                 , { 0x0f, 0x4b       }, 2, 1, 0, 0 }, /* /r    CMOVNP  r32,   r/m32       */
  { "cmovns "                 , { 0x0f, 0x49       }, 2, 1, 0, 0 }, /* /r    CMOVNS  r32,   r/m32       */
  { "cmovnz "                 , { 0x0f, 0x45       }, 2, 1, 0, 0 }, /* /r    CMOVNZ  r32,   r/m32       */
  { "cmovo  "                 , { 0x0f, 0x40       }, 2, 1, 0, 0 }, /* /r    CMOVO   r32,   r/m32       */
  { "cmovp  "                 , { 0x0f, 0x4a       }, 2, 1, 0, 0 }, /* /r    CMOVP   r32,   r/m32       */
  { "cmovpe "                 , { 0x0f, 0x4a       }, 2, 1, 0, 0 }, /* /r    CMOVPE  r32,   r/m32       */
  { "cmovpo "                 , { 0x0f, 0x4b       }, 2, 1, 0, 0 }, /* /r    CMOVPO  r32,   r/m32       */
  { "cmovs  "                 , { 0x0f, 0x48       }, 2, 1, 0, 0 }, /* /r    CMOVS   r32,   r/m32       */
  { "cmovz  "                 , { 0x0f, 0x44       }, 2, 1, 0, 0 }, /* /r    CMOVZ   r32,   r/m32       */
#if 0 /* crash, why? */
  { "popcnt"                 , { 0xf3, 0x0f, 0xb8  }, 3, 1, 0, 0 }, /* /r    POPCNT  r32,   r/m32       */
#endif
#if 0 /* NOTE: not really helpful... */
  /* TODO:  implement field for +rd and friends... */
  //0f ca                   bswap  %edx
  { "bswap %%eax"            , { 0x0f, 0xc8             }, 2, 0, 0, 0 }, /* +rd   BSWAP r32               */
  //{ "bswap %%edx"            , { 0x0f, 0xca             }, 1, 0, 0, 0 }, /* +rd   BSWAP r32               */
#endif
#if 0 /* DAMMIT; now if we divide by a register containing zero we get SIGFPE... */
  { "idiv"                   , { 0xf7                   }, 1, 1, 7, 0 }, /* /7    IDIV r/m32              */
#endif
  //{ 0, 1, 1, { 0x3b                         }, "cmp"                  },
#if 0
  /* NOTE: these now work, except almost always result
           in a never-ending program. either we give them
           a real short timeslice in which to run, or we
           need to verify that the contents of the loop
           DO something :-/ */
  { 1, 0, 1, { 0x74       }, "je"                 },
  { 1, 0, 1, { 0x75       }, "jnz"                },
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
  assert(0 == strncmp("or",      X86[OR_R32]     .descr, 2));
  assert(0 == strncmp("cmpxchg", X86[CMPXCHG_R32].descr, 7));
  assert(0 == strncmp("cmova",   X86[CMOVA]      .descr, 5));
  assert(0 == strncmp("cmovge",  X86[CMOVGE]     .descr, 6));
  assert(0 == strncmp("cmovne",  X86[CMOVNE]     .descr, 6));
  assert(0 == strncmp("cmovz",   X86[CMOVZ]      .descr, 5));
}

