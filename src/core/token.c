/*  praxis: services for TSora IRC networks.
 *  src/token.c: Token table routines.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "ilog.h"
#include "token.h"

/* token_to_value()
 *     Returns a token's value.
 *
 * inputs     - token table, token name
 * outputs    - token's value, or TOKEN_UNMATCHED on not found
 */
char
token_to_value(Token token_table[], const char *token)
{
    int i;

    iassert(token_table != NULL);
    iassert(token != NULL);

    for (i = 0; token_table[i].text != NULL; i++)
        if (!strcasecmp(token_table[i].text, token))
            return token_table[i].value;

    return TOKEN_UNMATCHED;
}

/* value_to_token()
 *     Returns a value's token.
 *
 * inputs     - token table, token value
 * outputs    - value's token or NULL on failure
 */
const char *
value_to_token(Token token_table[], int value)
{
    int i;

    iassert(token_table != NULL);

    for (i = 0; token_table[i].text != NULL; i++)
        if (token_table[i].value == value)
            return token_table[i].text;

    return NULL;
}
