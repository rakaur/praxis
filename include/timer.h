/*  praxis: services for TSora IRC networks.
 *  include/timer.h: Contains forward declarations for timer.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_timer_h
#define INCLUDED_timer_h

#define MAX_TIMERS 50

typedef void TMR (void *);
typedef struct Timer Timer;

struct Timer
{
    TMR *func;
    void *arg;
    const char *name;
    time_t frequency;
    time_t when;
    uchar active;
};

void timerInit(void);
void timerAdd(const char *, TMR *, void *, time_t);
void timerAddOnce(const char *, TMR *, void *, time_t);
int timerFind(TMR *, void *);
uchar timerDelete(TMR *, void *);
time_t timerNextRun(void);
void timerRun(void);

#endif /* INCLUDED_timer_h */
