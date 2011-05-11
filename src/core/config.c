/*  praxis: services for TSora IRC networks.
 *  src/config.c: Configuration file parser.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "configparse.h"
#include "dlink.h"
#include "balloc.h"
#include "connection.h"
#include "ilog.h"
#include "imem.h"
#include "module.h"
#include "token.h"
#include "uplink.h"
#include "config.h"

static uchar c_serverinfo(ConfigEntry *);
static uchar c_si_name(ConfigEntry *);
static uchar c_si_description(ConfigEntry *);
static uchar c_si_sid(ConfigEntry *);
static uchar c_si_vhost(ConfigEntry *);
static uchar c_si_reconnect_time(ConfigEntry *);
static uchar c_si_ping_time(ConfigEntry *);
static uchar c_si_admin_name(ConfigEntry *);
static uchar c_si_admin_email(ConfigEntry *);

static uchar c_uplink(ConfigEntry *);

static uchar c_userserv(ConfigEntry *);
static uchar c_us_enable(ConfigEntry *);
static uchar c_us_nick(ConfigEntry *);
static uchar c_us_user(ConfigEntry *);
static uchar c_us_host(ConfigEntry *);
static uchar c_us_real(ConfigEntry *);
static uchar c_us_max_users(ConfigEntry *);
static uchar c_us_default_flags(ConfigEntry *);

static uchar c_chanserv(ConfigEntry *);
static uchar c_cs_enable(ConfigEntry *);
static uchar c_cs_nick(ConfigEntry *);
static uchar c_cs_user(ConfigEntry *);
static uchar c_cs_host(ConfigEntry *);
static uchar c_cs_real(ConfigEntry *);
static uchar c_cs_max_chans(ConfigEntry *);
static uchar c_cs_join_chans(ConfigEntry *);
static uchar c_cs_part_chans(ConfigEntry *);
static uchar c_cs_default_flags(ConfigEntry *);

static uchar c_modules(ConfigEntry *);
static uchar c_modules_path(ConfigEntry *);

/* *INDENT-OFF* */

static ConfigTable config_root_table[] =
{
    { "SERVERINFO", 1, c_serverinfo },
    { "UPLINK",     1, c_uplink     },
    { "USERSERV",   1, c_userserv   },
    { "CHANSERV",   1, c_chanserv   },
    { "MODULES",    1, c_modules    },
    { NULL,         0, NULL         }
};

static ConfigTable config_si_table[] =
{
    { "NAME",           0, c_si_name           },
    { "DESCRIPTION",    0, c_si_description    },
    { "SID",            0, c_si_sid            },
    { "VHOST",          0, c_si_vhost          },
    { "RECONNECT_TIME", 1, c_si_reconnect_time },
    { "PING_TIME",      1, c_si_ping_time      },
    { "ADMIN_NAME",     1, c_si_admin_name     },
    { "ADMIN_EMAIL",    1, c_si_admin_email    },
    { NULL,             0, NULL                }
};

static ConfigTable config_us_table[] =
{
    { "ENABLE",        1, c_us_enable        },
    { "NICK",          1, c_us_nick          },
    { "USER",          0, c_us_user          },
    { "HOST",          0, c_us_host          },
    { "REAL",          0, c_us_real          },
    { "MAX_USERS",     1, c_us_max_users     },
    { "DEFAULT_FLAGS", 1, c_us_default_flags },
    { NULL,            0, NULL               }
};

static ConfigTable config_cs_table[] =
{
    { "ENABLE",        1, c_cs_enable        },
    { "NICK",          1, c_cs_nick          },
    { "USER",          0, c_cs_user          },
    { "HOST",          0, c_cs_host          },
    { "REAL",          0, c_cs_real          },
    { "MAX_CHANS",     1, c_cs_max_chans     },
    { "JOIN_CHANS",    1, c_cs_join_chans    },
    { "PART_CHANS",    1, c_cs_part_chans    },
    { "DEFAULT_FLAGS", 1, c_cs_default_flags },
    { NULL,            0, NULL               }
};

static ConfigTable config_modules_table[] =
{
    { "PATH", 1, c_modules_path },
    { NULL,   0, NULL           }
};

/* XXX uncomment this when i add chanserv/userserv
static Token config_uflags[] =
{
    { "HIDEMAIL", MU_HIDEMAIL },
    { "HOLD",     MU_HOLD     },
    { "NEVEROP",  MU_NEVEROP  },
    { "NOOP",     MU_NOOP     },
    { NULL,       0           }
};

static Token config_cflags[] =
{
    { "HOLD",    MC_HOLD    },
    { "NEVEROP", MC_NEVEROP },
    { "SECURE",  MC_SECURE  },
    { "VERBOSE", MC_VERBOSE },
    { NULL,      0          }
};*/

/* *INDENT-ON* */

/* configInit()
 *     Initialises the configuration settings.
 *
 * inputs     - none
 * outputs    - none
 */
void
configInit(void)
{
    /* Initialise serverinfo{} stuff. */
    me.name[0] = '\0';

    if (me.admin_name != NULL)
        free(me.admin_name);
    if (me.admin_email != NULL)
        free(me.admin_email);

    me.desc[0] = me.sid[0] = '\0';

    me.admin_name = me.admin_email = NULL;

    settings.vhost[0] = '\0';

    if (settings.network_name != NULL)
        free(settings.network_name);
    if (settings.mta_path != NULL)
        free(settings.mta_path);

    settings.network_name = settings.mta_path = NULL;
    settings.reconnect_time = settings.ping_time = settings.expire_time = 0;
    settings.auth_type = 0;

    globals.start = globals.max_fd = 0;
    globals.uplink_failed = globals.connected = globals.bursting = 0;

    globals.currtime = time(NULL);

    if (dlinkLength(&uplink_list) > 0)
        uplinkFlush();

    memset(&uplink_list, '\0', sizeof(uplink_list));

    /* Initialise userserv{} stuff. */
    userserv.nick[0] = userserv.user[0] = '\0';
    userserv.host[0] = userserv.real[0] = '\0';

    userserv.enabled = userserv.max_users = userserv.default_flags = 0;

    /* Initialise chanserv{} stuff. */
    chanserv.nick[0] = chanserv.user[0] = '\0';
    chanserv.host[0] = chanserv.real[0] = '\0';

    chanserv.max_chans = chanserv.default_flags = 0;
    chanserv.enabled = chanserv.join_chans = chanserv.part_chans = 0;
}

/* configParse()
 *     Parses the configuration file.
 *
 * inputs     - none
 * outputs    - none
 */
void
configParse(void)
{
    ConfigFile *cfptr, *cfp;
    ConfigEntry *ce;
    ConfigTable *ct = NULL;

    cfptr = cfp = config_load(globals.config_file);

    if (cfp == NULL)
    {
        ilog(L_INFO, "configParse(): Error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (; cfptr; cfptr = cfptr->cf_next)
    {
        for (ce = cfptr->cf_entries; ce; ce = ce->ce_next)
        {
            for (ct = config_root_table; ct->name; ct++)
            {
                if (!strcasecmp(ct->name, ce->ce_varname))
                {
                    if ((globals.run_flags & RF_REHASHING) &&
                        (ct->rehashable == 0))
                        continue;

                    ct->func(ce);
                    break;
                }
            }

            if (ct->name == NULL)
            {
                ilog(L_ERROR, "%s:%d: Invalid configuration option: %s",
                     ce->ce_fileptr->cf_filename, ce->ce_varlinenum,
                     ce->ce_varname);
            }
        }
    }

    config_free(cfp);
}

/* configCheck()
 *     Performs sanity checking on configuration settings.
 *
 * inputs     - none
 * outputs    - 1 on verified or 0 on failure
 */
uchar
configCheck(void)
{
    if (me.name[0] == '\0')
    {
        ilog(L_INFO, "Config error: no serverinfo::name set.");
        return 0;
    }

    if (me.desc[0] == '\0')
    {
        ilog(L_INFO, "Config error: no serverinfo::description set.");
        return 0;
    }

    if (me.sid == '\0')
    {
        ilog(L_INFO, "Config error: no serverinfo::sid set.");
        return 0;
    }

    if (settings.reconnect_time < 5)
    {
        ilog(L_INFO, "Config warning: value too low for "
             "serverinfo::reconnect_time; defaulting to 5.");
    }

    if (settings.ping_time == 0)
    {
        ilog(L_INFO, "Config warning: value too low for "
             "serverinfo::ping_time; defaulting to 5.");
    }

    if (me.admin_name == NULL)
    {
        ilog(L_INFO, "Config error: no serverinfo::admin_name set.");
        return 0;
    }

    if (me.admin_email == NULL)
    {
        ilog(L_INFO, "Config error: no serverinfo::admin_email set.");
        return 0;
    }

    /* This spits out a warning and not an error because other services
     * later on might want to use MyUsers.  Making it error out is probably
     * a bad idea, while spitting out a warning could serve as a reminder for
     * someone that didn't mean to do this.
     */
    if ((userserv.enabled == 1) && (chanserv.enabled == 0))
    {
        ilog(L_INFO, "Config warning: userserv is enabled without chanserv.");
    }

    if ((userserv.enabled == 0) && (chanserv.enabled == 1))
    {
        ilog(L_INFO, "Config error: chanserv is enabled without userserv.");
        return 0;
    }

    if (userserv.enabled == 1)
    {
        if (userserv.nick[0] == '\0')
        {
            ilog(L_INFO, "Config error: no userserv::nick set.");
            return 0;
        }

        if (userserv.user[0] == '\0')
        {
            ilog(L_INFO, "Config error: no userserv::user set.");
            return 0;
        }

        if (userserv.host[0] == '\0')
        {
            ilog(L_INFO, "Config error: no userserv::host set.");
            return 0;
        }

        if (userserv.real[0] == '\0')
        {
            ilog(L_INFO, "Config error: no userserv::real set.");
            return 0;
        }

        if (userserv.max_users < 1)
        {
            ilog(L_INFO, "Config warning: value too low for "
                 "userserv::max_users; defaulting to 5.");
        }
    }

    if (chanserv.enabled == 1)
    {
        if (chanserv.nick[0] == '\0')
        {
            ilog(L_INFO, "Config error: no chanserv::nick set.");
            return 0;
        }

        if (chanserv.user[0] == '\0')
        {
            ilog(L_INFO, "Config error: no chanserv::user set.");
            return 0;
        }

        if (chanserv.host[0] == '\0')
        {
            ilog(L_INFO, "Config error: no chanserv::host set.");
            return 0;
        }

        if (chanserv.real[0] == '\0')
        {
            ilog(L_INFO, "Config error: no chanserv::real set.");
            return 0;
        }

        if (chanserv.max_chans < 1)
        {
            ilog(L_INFO, "Config warning: value too low for "
                 "chanserv::max_users; defaulting to 5.");
        }
    }

    return 1;
}

static uchar
c_subblock(ConfigEntry *ce, const char *subblock, ConfigTable *table)
{
    ConfigTable *ct = NULL;

    for (ce = ce->ce_entries; ce; ce = ce->ce_next)
    {
        for (ct = table; ct->name; ct++)
        {
            if (!strcasecmp(ct->name, ce->ce_varname))
            {
                if ((globals.run_flags & RF_REHASHING) && (ct->rehashable == 0))
                    continue;

                ct->func(ce);
                break;
            }
        }

        if (ct->name == NULL)
        {
            ilog(L_ERROR, "%s:%d: Invalid configuration option: %s::%s",
                 ce->ce_fileptr->cf_filename, ce->ce_varlinenum,
                 subblock, ce->ce_varname);
        }
    }

    return 0;
}

static uchar
c_serverinfo(ConfigEntry *ce)
{
    c_subblock(ce, "serverinfo", config_si_table);
    return 0;
}

static uchar
c_si_name(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    strlcpy(me.name, ce->ce_vardata, HOSTLEN);

    return 0;
}

static uchar
c_si_description(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    strlcpy(me.desc, ce->ce_vardata, REALLEN);

    return 0;
}

static uchar
c_si_sid(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    strlcpy(me.sid, ce->ce_vardata, 4);

    return 0;
}

static uchar
c_si_vhost(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    strlcpy(settings.vhost, ce->ce_vardata, HOSTLEN);

    return 0;
}

static uchar
c_si_reconnect_time(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    settings.reconnect_time = ce->ce_vardatanum;

    return 0;
}

static uchar
c_si_ping_time(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    settings.ping_time = (ce->ce_vardatanum * 60);

    return 0;
}

static uchar
c_si_admin_name(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    me.admin_name = istrdup(ce->ce_vardata);

    return 0;
}

static uchar
c_si_admin_email(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    me.admin_email = istrdup(ce->ce_vardata);

    return 0;
}

static uchar
c_uplink(ConfigEntry *ce)
{
    Uplink *uplink_p;
    char name[HOSTLEN + 1], host[HOSTLEN + 1];
    char pass[PASSLEN + 1], vhost[HOSTLEN + 1];
    uint port = 0;

    name[0] = host[0] = pass[0] = vhost[0] = '\0';

    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    strlcpy(name, ce->ce_vardata, HOSTLEN);

    for (ce = ce->ce_entries; ce; ce = ce->ce_next)
    {
        if (!strcasecmp(ce->ce_varname, "HOST"))
        {
            if (ce->ce_vardata == NULL)
                PARAM_ERROR(ce);

            strlcpy(host, ce->ce_vardata, HOSTLEN);

            continue;
        }

        else if (!strcasecmp(ce->ce_varname, "VHOST"))
        {
            if (ce->ce_vardata == NULL)
                PARAM_ERROR(ce);

            strlcpy(vhost, ce->ce_vardata, HOSTLEN);

            continue;
        }

        else if (!strcasecmp(ce->ce_varname, "PASSWORD"))
        {
            if (ce->ce_vardata == NULL)
                PARAM_ERROR(ce);

            strlcpy(pass, ce->ce_vardata, PASSLEN);

            continue;
        }

        else if (!strcasecmp(ce->ce_varname, "PORT"))
        {
            if (ce->ce_vardata == NULL)
                PARAM_ERROR(ce);

            port = ce->ce_vardatanum;

            continue;
        }
        else
        {
            ilog(L_ERROR, "%s:%d: Invalid configuration option: uplink::%s",
                 ce->ce_fileptr->cf_filename, ce->ce_varlinenum,
                 ce->ce_varname);

            continue;
        }
    }

    /* Make sure it's all valid. */
    if (host[0] == '\0')
    {
        ilog(L_INFO, "Invalid uplink{} block: no host specified.");

        return 1;
    }

    if (pass[0] == '\0')
    {
        ilog(L_INFO, "Invalid uplink{} block: no pass specified.");

        return 1;
    }

    if (port == 0)
        port = 6667;

    /* If we get to here then it's good to add. */
    uplink_p = uplinkAdd(name, host, pass, port);

    strlcpy(uplink_p->vhost, vhost, HOSTLEN);

    return 0;
}

static uchar
c_userserv(ConfigEntry *ce)
{
    c_subblock(ce, "userserv", config_us_table);
    return 0;
}

static uchar
c_us_enable(ConfigEntry *ce)
{
    userserv.enabled = 1;
    return 0;
}

static uchar
c_us_nick(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (userserv.enabled == 0)
        return 1;

    strlcpy(userserv.nick, ce->ce_vardata, NICKLEN);

    return 0;
}

static uchar
c_us_user(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (userserv.enabled == 0)
        return 1;

    strlcpy(userserv.user, ce->ce_vardata, USERLEN);

    return 0;
}

static uchar
c_us_host(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (userserv.enabled == 0)
        return 1;

    strlcpy(userserv.host, ce->ce_vardata, HOSTLEN);

    return 0;
}

static uchar
c_us_real(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (userserv.enabled == 0)
        return 1;

    strlcpy(userserv.real, ce->ce_vardata, REALLEN);

    return 0;
}

static uchar
c_us_max_users(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (userserv.enabled == 0)
        return 1;

    userserv.max_users = ce->ce_vardatanum;

    return 0;
}

static uchar
c_us_default_flags(ConfigEntry *ce)
{
    if (userserv.enabled == 0)
        return 1;

    /* XXX - uncomment this when i do chanserv/userserv
       for (flce = ce->ce_entries; flce; flce = flce->ce_next)
       {
       int val;

       val = token_to_value(config_uflags, flce->ce_varname);

       if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
       userserv.default_flags |= val;

       else
       {
       ilog(L_INFO, "%s:%d: unknown flag: %s",
       flce->ce_fileptr->cf_filename, flce->ce_varlinenum,
       flce->ce_varname);
       }
       } */

    return 0;
}

static uchar
c_chanserv(ConfigEntry *ce)
{
    c_subblock(ce, "chanserv", config_cs_table);
    return 0;
}

static uchar
c_cs_enable(ConfigEntry *ce)
{
    chanserv.enabled = 1;
    return 0;
}

static uchar
c_cs_nick(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (chanserv.enabled == 0)
        return 1;

    strlcpy(chanserv.nick, ce->ce_vardata, NICKLEN);

    return 0;
}

static uchar
c_cs_user(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (chanserv.enabled == 0)
        return 1;

    strlcpy(chanserv.user, ce->ce_vardata, USERLEN);

    return 0;
}

static uchar
c_cs_host(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (chanserv.enabled == 0)
        return 1;

    strlcpy(chanserv.host, ce->ce_vardata, HOSTLEN);

    return 0;
}

static uchar
c_cs_real(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (chanserv.enabled == 0)
        return 1;

    strlcpy(chanserv.real, ce->ce_vardata, REALLEN);

    return 0;
}

static uchar
c_cs_max_chans(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

    if (chanserv.enabled == 0)
        return 1;

    chanserv.max_chans = ce->ce_vardatanum;

    return 0;
}

static uchar
c_cs_join_chans(ConfigEntry *ce)
{
    if (chanserv.enabled == 0)
        return 1;

    chanserv.join_chans = 1;

    return 0;
}

static uchar
c_cs_part_chans(ConfigEntry *ce)
{
    if (chanserv.enabled == 0)
        return 1;

    chanserv.part_chans = 1;

    return 0;
}

static uchar
c_cs_default_flags(ConfigEntry *ce)
{
    if (chanserv.enabled == 0)
        return 1;

/* XXX uncomment this when i do chanserv/userserv
    for (flce = ce->ce_entries; flce; flce = flce->ce_next)
    {
        int val;

        val = token_to_value(config_cflags, flce->ce_varname);

        if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
            chanserv.default_flags |= val;

        else
        {
            ilog(L_INFO, "%s:%d: unknown flag: %s",
                 flce->ce_fileptr->cf_filename, flce->ce_varlinenum,
                 flce->ce_varname);
        }
    }*/

    return 0;
}

static uchar
c_modules(ConfigEntry *ce)
{
    c_subblock(ce, "modules", config_modules_table);
    return 0;
}

static uchar
c_modules_path(ConfigEntry *ce)
{
    if (ce->ce_vardata == NULL)
        PARAM_ERROR(ce);

#ifndef STATIC_MODULES
    modulePathAdd(ce->ce_vardata);
#endif

    return 0;
}
