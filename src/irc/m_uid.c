/*  praxis: services for TSora IRC networks.
 *  src/irc/m_uid.c: Handles IRC's UID message.
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
#include "events.h"
#include "event.h"
#include "ilog.h"
#include "irc.h"
#include "send.h"
#include "server.h"
#include "uplink.h"
#include "user.h"

/* :<origin> UID <nick> <hops> <ts> +<modes> <user> <host> <ip> <uid> :<gecos>
 *
 * parv[0] = nickname
 * parv[1] = hops (not used)
 * parv[2] = timestamp
 * parv[3] = modes
 * parv[4] = username
 * parv[5] = hostname
 * parv[6] = IP address (not used)
 * parv[7] = UID
 * parv[8] = real name (not used)
 */
void
m_uid(char *origin, uint parc, char *parv[])
{
    Server *server_p;
    User *user_p;

    if (curr_uplink->ts_version != 6)
    {
        ilog(L_DEBUG2, "m_uid(): Got TS6 UID introduction in TS5?");
        return;
    }

    /* Verify the message. */
    if (parc != 9)
    {
        ilog(L_ERROR, "m_uid(): Received only %d parameters; expecting 9",
             parc);

        sendIRC(curr_uplink->connection_p,
                "ERROR :Closing Link: %s %s (Invalid UID command.)",
                curr_uplink->name, curr_uplink->host);

        connectionDead(curr_uplink->connection_p);

        return;
    }

    server_p = serverFindSID(origin);

    if (server_p == NULL)
    {
        ilog(L_DEBUG2, "m_uid(): Got user on nonexistant server: %s -> %s",
             parv[0], origin);

        sendIRC(curr_uplink->connection_p,
                "ERROR :Closing Link: %s %s (Unknown server: %s)",
                curr_uplink->name, curr_uplink->host, origin);

        connectionDead(curr_uplink->connection_p);

        return;
    }

    ilog(L_DEBUG2, "m_uid(): New user: %s!%s@%s[%s] -> %s[%s]",
         parv[0], parv[4], parv[5], parv[7], server_p->name, origin);

    /*               nick     user     host     server    UID */
    user_p = userAdd(parv[0], parv[4], parv[5], server_p, parv[7]);
    user_p->timestamp = atoi(parv[2]);

    eventPost(E_NEWUSER, user_p);

    return;
}
