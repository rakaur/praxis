/*  praxis: services for TSora IRC networks.
 *  include/uplink.h: Contains forward declarations for uplink.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_uplink_h
#define INCLUDED_uplink_h

typedef struct Uplink Uplink;

struct Uplink
{
    char name[HOSTLEN + 1];     /* the server name */
    char host[HOSTLEN + 1];     /* connection address */
    char pass[PASSLEN + 1];     /* IRC password */
    char vhost[HOSTLEN + 1];    /* ip or host to bind to */
    char sid[6];                /* TS6 server id */
    uint port;                  /* connection port */
    uint ts_version;            /* version of the TS protocol */

    Connection *connection_p;   /* uplink's Connection (or NULL) */
};

DLinkList uplink_list;
Uplink *curr_uplink;

#define uplinkFindDelete(name) uplinkDelete(uplinkFind(name))

void uplinkInit(void);
Uplink *uplinkAdd(const char *, const char *, const char *, uint);
Uplink *uplinkFind(const char *);
uchar uplinkDelete(Uplink *);
uchar uplinkFlush(void);
Connection *uplinkConnect(void);

#endif /* INCLUDED_uplink_h */
