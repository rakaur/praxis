/*  praxis: services for TSora IRC networks.
 *  src/irc/m_kill.c: Handles IRC's KILL message.
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

/* :<origin> KILL <target> :<path/reason>
 *
 * parv[0] = target
 * parv[1] = path/reason (not used)
 */
void
m_kill(char *origin, uint parc, char *parv[])
{
    if (parc > 2)
    {
        ilog(L_ERROR, "m_kill(): Got user kill with too few parameters");

        return;
    }

    ilog(L_DEBUG2, "m_kill(): User killed: %s by %s", parv[0], origin);

    if (curr_uplink->ts_version == 6)
        userFindUIDDelete(parv[0]);
    else
        userFindNickDelete(parv[0]);

    /* XXX - check to see if they killed our clients */
}
