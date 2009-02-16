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

#if 0
typedef int (*func)(int);
#else
typedef float (*func)(float);
#endif

#undef  PRIx8
#define PRIx8 "hhx"
#undef  PRIu8
#define PRIu8 "hhu"

#endif
