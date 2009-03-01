/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */

#ifndef RND_H
#define RND_H

#include <stdlib.h>
#include "typ.h"

#ifdef WIN32
float drand48(void);
#endif

void rnd32_init(u32);
#if 1
u32  rnd32(void);
#else
//# define rnd32() random()
#endif

u32  randr(u32 min, u32 max);
void randr_test(void);

/**
 * return a random double within the range [0.0, 1.0)
 */
//#define rand01()  drand48()

float rand01(void);
float randfr(float min, float max);

#endif

