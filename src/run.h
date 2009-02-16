/* ex: set ff=dos ts=2 et: */
/* $Id$ */
/*
 * Copyright 2009 Ryan Flynn <URL: http://www.parseerror.com/>
 *
 * Released under the MIT License, see the "LICENSE.txt" file or
 *   <URL: http://www.opensource.org/licenses/mit-license.php>
 */
/*
 *
 */

#ifndef RUN_H
#define RUN_H

#include "typ.h"

#ifdef X86_USE_FLOAT
float score(const void *f, int verbose);
#else
u32   score(const void *f, int verbose);
#endif
u32 popcnt(u32 n);

#endif

