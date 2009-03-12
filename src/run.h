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
#include "gen.h"

void run_init(void);
void score(genoscore *, const genx_iface *, int verbose);

#endif

