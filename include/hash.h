/*  praxis: services for TSora IRC networks.
 *  include/hash.h: Contains forward declarations for hash.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_hash_h
#define INCLUDED_hash_h

/* NOTE: I don't really understand the following magic.  Most of these routines
 * are based on ircd-ratbox's, so kudos to them.  Hopefully these should be
 * supremely fast with low collision rates.
 *
 * Hash sizes are in praxis.h.
 */

#define FNV1_32_INIT 0x811c9dc5UL

uint hashUpper(const char *, int);
uint hashUpperLen(const char *, int, int);
uint hash(const char *, int);
uint hashLen(const char *, int, int);

uint hashServer(const char *);
uint hashNick(const char *);
uint hashChannel(const char *);

uint hashCmd(const char *);

#endif /* INCLUDED_hash_h */
