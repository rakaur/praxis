/*  praxis: services for TSora IRC networks.
 *  src/irc/m_pass.c: Handles IRC's PASS message.
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

/* TS6:  PASS <password> TS 6 :<SID>
 * ELSE: PASS <password> :TS
 *
 * TS6:
 *     parv[0] = password
 *     parv[1] = "TS"
 *     parv[2] = "6"
 *     parv[3] = SID
 * ELSE:
 *     parv[0] = password
 *     parv[1] = "TS"
 */
void
m_pass(char *origin, uint parc, char *parv[])
{
    /* Check to see that it is a TS server. */
    if (parc < 2)
    {
        ilog(L_ERROR, "m_pass(): Non-TS server %s", curr_uplink->name);

        sendIRC(curr_uplink->connection_p,
                "ERROR :Closing Link: %s (Non-TS server)", curr_uplink->host);

        connectionDead(curr_uplink->connection_p);

        return;
    }

    /* This is the TS6 PASS. */
    if (parc == 4)
    {
        ilog(L_DEBUG2, "m_pass(): %s is running in TS6", curr_uplink->name);
        curr_uplink->ts_version = 6;
        strlcpy(curr_uplink->sid, parv[3], 4);
    }

    /* Check to see that it matches. */
    if (strcmp(curr_uplink->pass, parv[0]))
    {
        ilog(L_ERROR, "m_pass(): Password mismatch for %s (%s != %s)",
             curr_uplink->name, parv[0], curr_uplink->pass);

        sendIRC(curr_uplink->connection_p,
                "ERROR :Closing Link: %s %s (Invalid password.)",
                curr_uplink->host, curr_uplink->name);

        connectionDead(curr_uplink->connection_p);
    }

    /* Add our server. */
    serverAdd(me.name, (curr_uplink->ts_version == 6) ? me.sid : NULL);
}
