/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */

#ifndef TYP_H
#define TYP_H

#include <stdint.h>
#include <inttypes.h>

typedef  uint8_t  u8;
typedef   int8_t  s8;
typedef  int32_t s32;
typedef uint32_t u32;
typedef uint64_t u64;

#ifdef X86_USE_FLOAT
# define PRIt "g"
struct target {
  float in[3],
        out;
};
#else
# define PRIt "u"
struct target {
  s32 in[3],
      out;
};
#endif

#if defined(__GNUC__) && !(defined(__STRICT_ANSI__))
/*
 * libc defines these as "x" and "u", but this results in values
 * not being truncated to 8 bits
 */
# undef  PRIx8
# define PRIx8 "hhx"
# undef  PRIu8
# define PRIu8 "hhu"
#endif

#endif /* TYP_H */

