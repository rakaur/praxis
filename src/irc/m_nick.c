/*  praxis: services for TSora IRC networks.
 *  src/irc/m_nick.c: Handles IRC's NICK message.
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
#include "hash.h"
#include "ilog.h"
#include "irc.h"
#include "send.h"
#include "server.h"
#include "uplink.h"
#include "user.h"

/* INTRODUCTION (in TS5):
 *     NICK <nick> <hops> <ts> +<modes> <user> <host> <server> :<gecos>
 * CHANGE:
 *     :<origin> NICK <nick> :<ts>
 *
 * INTRODUCTION (in TS5):
 *     parv[0] = nickname
 *     parv[1] = hops (not used)
 *     parv[2] = timestamp
 *     parv[3] = modes
 *     parv[4] = username
 *     parv[5] = hostname
 *     parv[6] = server name
 *     parv[7] = real name (not used)
 *
 * CHANGE:
 *     parv[0] = nickname
 *     parv[1] = timestamp
 */
void
m_nick(char *origin, uint parc, char *parv[])
{
    Server *server_p;
    User *user_p;

    /* Verify the message. */
    if ((parc != 8) && (parc != 2))
    {
        ilog(L_ERROR, "m_nick(): Received only %d parameters; expecting 8 or 2",
             parc);

        sendIRC(curr_uplink->connection_p,
                "ERROR :Closing Link: %s %s (Invalid NICK command.)",
                curr_uplink->name, curr_uplink->host);

        connectionDead(curr_uplink->connection_p);

        return;
    }

    /* This is a user introduction in TS5. */
    if (parc == 8)
    {
        if (curr_uplink->ts_version == 6)
        {
            ilog(L_DEBUG2, "m_nick(): Got NICK introduction in TS6?");
            return;
        }

        server_p = serverFindName(parv[6]);

        if (server_p == NULL)
        {
            ilog(L_DEBUG2, "m_nick(): Got user on nonexistant server: %s -> %s",
                 parv[0], parv[6]);

            sendIRC(curr_uplink->connection_p,
                    "ERROR :Closing Link: %s %s (Unknown server: %s)",
                    curr_uplink->name, curr_uplink->host, parv[6]);

            connectionDead(curr_uplink->connection_p);

            return;
        }

        ilog(L_DEBUG2, "m_nick(): New user: %s!%s@%s -> %s",
             parv[0], parv[4], parv[5], parv[6]);

        /*               nick     user     host     server    UID */
        user_p = userAdd(parv[0], parv[4], parv[5], server_p, NULL);
        user_p->timestamp = atoi(parv[2]);

        eventPost(E_NEWUSER, user_p);

        return;
    }

    /* This is a nickname change. */
    else if (parc == 2)
    {
        if (curr_uplink->ts_version == 6)
            user_p = userFindUID(origin);
        else
            user_p = userFindNick(origin);

        iassert(user_p != NULL);

        ilog(L_DEBUG2, "m_nick(): Nickname change: %s -> %s", origin, parv[0]);

        /* Remove the current one from the user_nick_hash. */
        dlinkFindDestroy(user_p, &user_nick_hash[user_p->nick_hash]);

        /* Change the nick, update the TS, rehash, and readd. */
        strlcpy(user_p->nick, parv[0], NICKLEN);
        user_p->timestamp = atoi(parv[1]);
        user_p->nick_hash = hashNick(parv[0]);

        dlinkAddAlloc(user_p, &user_nick_hash[user_p->nick_hash]);

        return;
    }
}
