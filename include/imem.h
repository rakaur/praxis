/*  praxis: services for TSora IRC networks.
 *  include/imem.h: Contains forward declarations for imem.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_imem_h
#define INCLUDED_imem_h

void *imalloc(long);
void *icalloc(long, long);
void *irealloc(void *, long);
char *istrdup(const char *);

#endif /* INCLUDED_imem_h */
