/*  praxis: services for TSora IRC networks.
 *  src/irc/m_error.c: Handles IRC's ERROR message.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "balloc.h"
#include "connection.h"
#include "ilog.h"
#include "irc.h"
#include "send.h"
#include "uplink.h"

/* ERROR :<error>
 *
 * parv[0] = error string
 */
void
m_error(char *origin, uint parc, char *parv[])
{
    ilog(L_INFO, "m_error(): Error from server: %s", parv[0]);
    connectionDead(curr_uplink->connection_p);
}
