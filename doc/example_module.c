/*  praxis: services for TSora IRC networks.
 *  doc/example_module.c: Example module documentation.
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
#include "uplink.h"

static uchar moduleInit(void);
static void moduleFini(void);
static void m_error(char *, uint, char **);

ModuleHeader error_header = { "m_error.so", moduleInit, moduleFini };

CmdHashEntry error_tab = { "ERROR", AC_NA, 0, m_error };

static uchar
moduleInit(void)
{
    cmdhashAdd(&error_tab, irc_cmd_hash);
    return 1;
}

static void
moduleFini(void)
{
    cmdhashDelete(&error_tab, irc_cmd_hash);
}


/* ERROR :<error>
 *
 * parv[0] = error string
 */
static void
m_error(char *origin, uint parc, char *parv[])
{
    ilog(L_INFO, "m_error(): Error from server: %s", parv[0]);
    connectionDead(curr_uplink->connection_p);
}

/* XXX - do me */
