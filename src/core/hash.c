/*  praxis: services for TSora IRC networks.
 *  src/hash.c: Fowler/Noll/Vo hashing routines.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *  Copyright (c) 2002-2004 ircd-ratbox development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "ilog.h"
#include "hash.h"

/* hashUpper()
 *     Converts a string to upper case and hashes the specified bits.
 *
 * inputs     - string to hash, bits
 * outputs    - hashed value
 */
uint
hashUpper(const char *s, int bits)
{
    uint h = FNV1_32_INIT;

    iassert(s != NULL);

    while (*s)
    {
        h ^= toupper(*s++);     /* XXX rfc case mapping */
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }

    h = (h >> bits) ^ (h & ((2 ^ bits) - 1));

    return h;
}

/* hashUpperLen()
 *     Converts a string to upper case and hashes the specified length.
 *
 * inputs     - string to hash, bits, len
 * outputs    - hashed value
 */
uint
hashUpperLen(const char *s, int bits, int len)
{
    uint h = FNV1_32_INIT;
    const char *x = (s + len);

    iassert(s != NULL);

    while ((*s) && (s < x))
    {
        h ^= toupper(*s++);     /* XXX rfc case mapping */
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }

    h = (h >> bits) ^ (h & ((2 ^ bits) - 1));

    return h;
}

/* hash()
 *     Hashes the a specified string.
 *
 * inputs     - string to hash, bits
 * outputs    - hashed value
 */
uint
hash(const char *s, int bits)
{
    uint h = FNV1_32_INIT;

    iassert(s != NULL);

    while (*s)
    {
        h ^= *s++;
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }

    h = (h >> bits) ^ (h & ((2 ^ bits) - 1));

    return h;
}

/* hashLen()
 *     Hashes the specified part of a string.
 *
 * inputs     - string to hash, bits, len
 * outputs    - hashed value
 */
uint
hashLen(const char *s, int bits, int len)
{
    uint h = FNV1_32_INIT;
    const char *x = (s + len);

    iassert(s != NULL);

    while ((*s) && (s < x))
    {
        h ^= *s++;
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }

    h = (h >> bits) ^ (h & ((2 ^ bits) - 1));

    return h;
}

/* hashServer()
 *     Hashes a server name.
 *
 * inputs     - string to hash
 * outputs    - hashed value
 */
uint
hashServer(const char *s)
{
    return hashUpper(s, SERVER_HASH_BITS);
}

/* hashNick()
 *     Hashes a nickname.
 *
 * inputs     - string to hash
 * outputs    - hashed value
 */
uint
hashNick(const char *s)
{
    return hashUpper(s, USER_HASH_BITS);
}

/* hashChannel()
 *     Hashes a channel name.
 *
 * inputs     - string to hash
 * outputs    - hashed value
 */
uint
hashChannel(const char *s)
{
    return hashUpperLen(s, CHANNEL_HASH_BITS, 30);
}

/* hashCmd()
 *     A horrible hashing function for command hash tables.
 *
 * inputs     - string to hash
 * outputs    - hashed value
 */
uint
hashCmd(const char *s)
{
    long h = 0, g;

    while (*s)
    {
        h = (h << 4) + tolower(*s++);   /* XXX rfc casemapping */

        if ((g = (h & 0xF0000000)))
            h ^= g >> 24;

        h &= ~g;
    }

    return (h % CMD_HASH_SIZE);
}
