/*  praxis: services for TSora IRC networks.
 *  include/sendq.h: Contains forward declarations for sendq.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_sendq_h
#define INCLUDED_sendq_h

typedef struct SendQ SendQ;

struct SendQ
{
    char buf[BUFSIZE];
    int len;
    int pos;
};

extern Heap *sendq_heap;

void sendqInit(void);
void sendqAdd(Connection *, const char *, int, int);
uchar sendqFlush(Connection *);

#endif /* INCLUDED_sendq_h */
