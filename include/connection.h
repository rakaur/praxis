/*  praxis: services for TSora IRC networks.
 *  include/connection.h: Contains forward declarations for connection.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_connection_h
#define INCLUDED_connection_h

typedef struct Connection Connection;

struct Connection
{
    char name[HOSTLEN + 1];     /* name associated with the connection */
    char hbuf[BUFSIZE + 1];     /* a buffer for the read handler */

    DLinkList sendq;            /* a sendq so we don't lose data */
    DLinkList recvq;            /* a recvq for the parser */

    int fd;                     /* the file descriptor */
    time_t first_data_on;       /* time we started this */
    time_t last_recv_on;        /* time we last got data */
    uchar pinged;               /* have we pinged it? */

    struct sockaddr_in *sa;     /* our peer name and such */

    uint flags;                 /* CF_ bitmask */
};

#define CF_UPLINK     0x00000001
#define CF_DCCOUT     0x00000002
#define CF_DCCIN      0x00000004

#define CF_CONNECTING 0x00000008
#define CF_LISTENING  0x00000010
#define CF_CONNECTED  0x00000020
#define CF_DEAD       0x00000040

DLinkList connection_list;
DLinkList dying_list;

Heap *sa_heap;

#define CF_IS_UPLINK(x) ((x)->flags & CF_UPLINK)
#define CF_IS_DCC(x) ((x)->flags & (CF_DCCOUT | CF_DCCIN))
#define CF_IS_DCCOUT(x) ((x)->flags & CF_DCCOUT)
#define CF_IS_DCCIN(x) ((x)->flags & CF_DCCIN)
#define CF_IS_DEAD(x) ((x)->flags & CF_DEAD)
#define CF_IS_CONNECTING(x) ((x)->flags & CF_CONNECTING)
#define CF_IS_LISTENING(x) ((x)->flags & CF_LISTENING)

void connectionInit(void);
Connection *connectionAdd(const char *, int, uint);
Connection *connectionFind(int);
uchar connectionDelete(Connection *);
void connectionDead(Connection *);

#endif /* INCLUDED_connection_h */
