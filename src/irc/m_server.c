/*  praxis: services for TSora IRC networks.
 *  src/irc/m_server.c: Handles IRC's SERVER message.
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

/* SERVER <name> <hops> :<gecos>
 *
 * parv[0] = name
 * parv[1] = hops
 * parv[2] = gecos
 */
void
m_server(char *origin, uint parc, char *parv[])
{
    if (parc > 3)
    {
        ilog(L_ERROR, "m_server(): Got server introduction with too few "
             "parameters");

        return;
    }

    if (cnt.server == 1)
    {
        if (strcasecmp(curr_uplink->name, parv[0]))
        {
            ilog(L_ERROR, "m_server(): Uplink name mismatch (%s != %s)",
                 parv[0], curr_uplink->name);

            sendIRC(curr_uplink->connection_p,
                    "ERROR :Closing Link: %s %s (Invalid servername.)",
                    curr_uplink->host, curr_uplink->name);

            connectionDead(curr_uplink->connection_p);

            return;
        }
    }

    serverAdd(parv[0],
              (curr_uplink->ts_version == 6) ? curr_uplink->sid : NULL);
}
