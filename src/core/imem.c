/*  praxis: services for TSora IRC networks.
 *  src/imem.c: Primitive memory allocation routines.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "imem.h"

/* imalloc()
 *     malloc()'s and error checks.
 *
 * inputs     - size to allocate
 * outputs    - pointer to newly allocated memory
 */
void *
imalloc(long size)
{
    void *buf = malloc(size);

    if (buf == NULL)
        raise(SIGUSR1);

    return buf;
}

/* icalloc()
 *     calloc()'s and error checks.
 *
 * inputs     - size to allocate, number to allocate
 * outputs    - pointer to newly allocated memory
 */
void *
icalloc(long elsize, long els)
{
    void *buf = calloc(elsize, els);

    if (buf == NULL)
        raise(SIGUSR1);

    return buf;
}

/* irealloc()
 *     realloc()'s and error checks.
 *
 * inputs     - pointer to old memory, new size to allocate
 * outputs    - pointer to newly allocated memory
 */
void *
irealloc(void *oldptr, long newsize)
{
    void *buf = realloc(oldptr, newsize);

    if (buf == NULL)
        raise(SIGUSR1);

    return buf;
}

/* istrdup()
 *     strdup()'s and error checks.
 *
 * inputs     - string
 * outputs    - pointer to string
 */
char *
istrdup(const char *s)
{
    char *t = imalloc(strlen(s) + 1);

    if (t == NULL)
        raise(SIGUSR1);

    strcpy(t, s);

    return t;
}
