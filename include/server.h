/*  praxis: services for TSora IRC networks.
 *  include/server.h: Contains forward declarations for server.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_server_h
#define INCLUDED_server_h

typedef struct Server Server;

struct Server
{
    char name[HOSTLEN + 1];     /* server name */
    char sid[6];                /* TS6 server id */

    DLinkList user_list;        /* list of users */

    uint flags;                 /* SF_ bitmask */
    uint name_hash;             /* hash id for quick reference */
    uint sid_hash;              /* hash id for quick reference */
};

#define SF_UPLINK 0x01
#define SF_HIDE   0x02

DLinkList server_name_hash[SERVER_HASH_SIZE];
DLinkList server_sid_hash[SERVER_HASH_SIZE];

#define serverFindNameDelete(name) serverDelete(serverFindName(name))
#define serverFindSIDDelete(sid) serverDelete(serverFindSID(sid))

void serverInit(void);
Server *serverAdd(const char *, const char *);
Server *serverFindName(const char *);
Server *serverFindSID(const char *);
uchar serverDelete(Server *);
uchar serverFlush(void);

#endif /* INCLUDED_server_h */
