/*  praxis: services for TSora IRC networks.
 *  include/event.h: Contains forward declarations for event.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_event_h
#define INCLUDED_event_h

typedef struct Event Event;

struct Event
{
    uchar (*first_func) (void *);       /* first function to be called */
    DLinkList func_list;        /* list of funcs to be called */
    uchar (*last_func) (void *);        /* last function to be called */
};

typedef struct EventQ EventQ;

struct EventQ
{
    uint event;                 /* E_event */
    void *arg;                  /* argument */
};

Event event_table[E_LAST];
DLinkList event_queue;

void eventInit(void);
void eventAdd(uint, uchar (*)());
uchar eventAddSpecial(uint, uchar, uchar (*)());
uchar eventDelete(uint, uchar (*)());
uchar eventDeleteSpecial(uint, uchar, uchar (*)());
void eventPost(uint, void *);
uchar eventShouldRun(void);
void eventRun(void);

#endif /* INCLUDED_event_h */
