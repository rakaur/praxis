/*  praxis: services for TSora IRC networks.
 *  include/token.h: Contains forward declarations for token.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_token_h
#define INCLUDED_token_h

#define TOKEN_UNMATCHED -1
#define TOKEN_ERROR -2

typedef struct Token Token;

struct Token
{
    const char *text;
    int value;
};

char token_to_value(Token *, const char *);
const char *value_to_token(struct Token *, int);

#endif /* INCLUDED_token_h */
