#include "praxis.h"
#include "irc.h"

static uchar moduleInit(void);
static void moduleFini(void);

extern void TEST_OMFG(char *);

ModuleHeader userserv_header = { "m_userserv.so", moduleInit, moduleFini };

static uchar
moduleInit(void)
{
    TEST_OMFG("hiFFUCKCCCCCCCCCCCCCCCCCCCC");
    return 1;
}

static void
moduleFini(void)
{
}
