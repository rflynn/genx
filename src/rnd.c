/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */

#include <assert.h>
#define _XOPEN_SOURCE /* drand48() via stdlib */
#include <stdlib.h>
#include "rnd.h"

#if 0
/*
 * just wrap built-ins srandom()/random()
 */
void rnd32_init(u32 seed)
{
  srandom(seed);
}
#endif

#if 1
/*
 * custom, unstrusted, fast bitwise prng from the web :/
 */
static u32 rndhi,
           rndlo;
/**
 * http://www.flipcode.com/archives/07-15-2002.shtml
 */
u32 rnd32(void)
{
  rndhi = (rndhi << 16) + (rndhi >> 16);
  rndhi += rndlo;
  rndlo += rndhi;
  return rndhi;
}

void rnd32_init(u32 seed)
{
  rndhi = seed;
  rndlo = rndhi ^ 0x49616E42;
}

#endif

#if 0

/**
 * Subject: Random numbers in C: Some suggestions.
 * Date: Tue, 12 Jan 1999 09:37:37 -0500
 * From: George Marsaglia <geo@stat.fsu.edu>
 * Message-ID: <369B5E30.65A55FD1@stat.fsu.edu>
 * Newsgroups: sci.stat.math,sci.math,sci.math.num-analysis,sci.crypt,sci.physics
 * Lines: 243
 * <URL: http://www.ciphersbyritter.com/NEWS4/RANDC.HTM#369B5E30.65A55FD1@stat.fsu.edu>
 * Inclusion of
 *
 *  ...
 *
 * together with suitably initialized seeds in
 *
 *  ...
 *
 * will allow you to put the string SWB in any C
 * expression and it will provide, in about 100 nanosecs,
 * a 32-bit random integer with period  2^7578. (Here
 * and below, ^ means exponent, except in C expressions,
 * where it means xor (exclusive-or).
 *
 */
#define rnd() (RndT[RndC+237]=(RndX=RndT[RndC+15])-(RndY=RndT[++RndC]+(RndX<RndY)))
static u32 RndT[256],
           RndX,
           RndY;
static u8  RndC;

static void rnd_init(u32 seed)
{
  unsigned i;
  srandom(seed ^ 0xDEADBEEF);
  RndX = seed;
  RndY = seed ^ 0xDECAFBAD;
  RndC = (u8)seed;
  for (i = 0; i < 256; i++)
    RndT[i] = random();
}

#endif

#if 0
/**
 * ranged random u32
 */
#define randr(min, max) ((min) + (rnd() % ((max) - (min) + 1)))

#else

/**
 * ranged [min, max] u32
 * NOTE: this function gets called ALOT
 *       make it faster(!)
 */
u32 randr(u32 min, u32 max)
{
#if 0
  /* this is the more proper way, but it's slower because it uses division! */
  double scaled = random() / (RAND_MAX + 1.0);
  u32 r = min + ((max - min + 1) * scaled);
#else
  /* trade quicker time for lower quality psuedo-random numbers */
  u32 r = min + (rnd32() % (max - min + 1));
#endif
  return r;
}

float randfr(float min, float max)
{
  float r = min + ((max - min + 1) * rand01());
  return r;
}

#endif

