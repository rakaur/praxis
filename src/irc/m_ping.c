/*  praxis: services for TSora IRC networks.
 *  src/irc/m_ping.c: Handles IRC's PING message.
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

/* PING :<origin>
 *
 * parv[0] = origin
 */
void
m_ping(char *origin, uint parc, char *parv[])
{
    Server *server_p;

    if (curr_uplink->ts_version == 6)
    {
        /* On the initial burst, we get a TS6 to emulate EOB.
         * However, all PINGs from there on out are TS5.
         * I'm not sure if that's "the right thing to do,"
         * but we'll try TS5 first and fall back to TS6.
         * We'll also send back a TS6 PONG regardless.
         *
         * androsyn tells me this is done because hybrid-7.1
         * chokes on SIDs in PING... still doesn't explain
         * the TS6 EOB PING though.
         *
         * In the long run this doesn't matter; any
         * activity on the line registers as a reply,
         * but it still bugs me.
         */
        server_p = serverFindName(parv[0]);

        if (server_p == NULL)
            server_p = serverFindSID(parv[0]);

        iassert(server_p != NULL);

        sendIRC(curr_uplink->connection_p,
                ":%s PONG %s :%s", me.sid, me.name, server_p->sid);
    }
    else
        sendIRC(curr_uplink->connection_p,
                ":%s PONG %s :%s", me.name, me.name, parv[0]);
}
