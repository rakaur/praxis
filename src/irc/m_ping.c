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
        server_p = serverFindSID(parv[0]);

        iassert(server_p != NULL);

        sendIRC(curr_uplink->connection_p,
                ":%s PONG %s :%s", me.sid, me.name, server_p->sid);
    }
    else
        sendIRC(curr_uplink->connection_p,
                ":%s PONG %s :%s", me.name, me.name, parv[0]);
}
