/*  praxis: services for TSora IRC networks.
 *  src/irc/m_sid.c: Handles IRC's SID message.
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
#include "server.h"
#include "uplink.h"

/* :<uplink sid> SID <name> <hops> <sid> :<gecos>
 *
 * parv[0] = name
 * parv[1] = hops (not used)
 * parv[2] = sid
 * parv[3] = gecos (not used)
 */
void
m_sid(char *origin, uint parc, char *parv[])
{
    if (parc > 4)
    {
        ilog(L_ERROR, "m_sid(): Got server introduction with too few "
             "parameters");

        return;
    }

    ilog(L_DEBUG2, "m_sid(): New server: %s[%s] (%s)",
         parv[0], parv[2], parv[3]);

    serverAdd(parv[0], parv[2]);
}
