/*  praxis: services for TSora IRC networks.
 *  src/irc/m_squit.c: Handles IRC's SQUIT message.
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
#include "user.h"

/* SQUIT <split> <uplink>
 *
 * parv[0] = split server
 * parv[1] = uplink server (not used)
 */
void
m_squit(char *origin, uint parc, char *parv[])
{
    Server *server_p;
    User *user_p;
    DLinkNode *node_p, *tnode_p;

    if (parc > 2)
    {
        ilog(L_ERROR, "m_squit(): Got server exit with too few parameters");

        return;
    }

    ilog(L_DEBUG2, "m_squit(): Server leaving: %s from %s", parv[0], parv[1]);

    if (curr_uplink->ts_version == 6)
        server_p = serverFindSID(parv[0]);
    else
        server_p = serverFindName(parv[0]);

    iassert(server_p != NULL);

    /* We need to remove all users on an exiting server to
     * comply with CAPAB QS.
     */
    DLINK_FOREACH_SAFE(node_p, tnode_p, server_p->user_list.head)
    {
        user_p = node_p->data;

        userDelete(user_p);
    }

    /* Now delete the server. */
    serverDelete(server_p);
}
