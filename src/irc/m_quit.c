/*  praxis: services for TSora IRC networks.
 *  src/irc/m_quit.c: Handles IRC's QUIT message.
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

/* :<origin> QUIT :<reason>
 *
 * parv[0] = reason (not used)
 */
void
m_quit(char *origin, uint parc, char *parv[])
{
    ilog(L_DEBUG2, "m_quit(): User leaving: %s", origin);

    if (curr_uplink->ts_version == 6)
        userFindUIDDelete(origin);
    else
        userFindNickDelete(origin);

    eventPost(E_USERQUIT, origin);
}
