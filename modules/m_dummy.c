#include "praxis.h"
#include "irc.h"

static uchar moduleInit(void);
static void moduleFini(void);
static void m_dummy(char *, uint, char **);

ModuleHeader dummy_header = { "m_dummy.so", moduleInit, moduleFini };

CmdHashEntry dummy_tab = { "DUMMY", AC_NA, 0, m_dummy };

static uchar
moduleInit(void)
{
    cmdhashAdd(&dummy_tab, irc_cmd_hash);
    return 1;
}

static void
moduleFini(void)
{
    cmdhashDelete(&dummy_tab, irc_cmd_hash);
}


static void
m_dummy(char *origin, uint parc, char *parv[])
{
    printf("DUMMY!\n");
}
