/*  praxis: services for TSora IRC networks.
 *  src/timer.c: Manages the various Timers.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *  Copyright (c) 2002-2004 ircd-ratbox development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "ilog.h"
#include "timer.h"

Timer timer_table[MAX_TIMERS];
static time_t timer_min = -1;

/* timerInit()
 *     Initialises the timer_table.
 *
 * inputs     - none
 * outputs    - none
 */
void
timerInit(void)
{
    globals.last_timer = NULL;
    memset(&timer_table, 0, sizeof(timer_table));
}

/* timerAdd()
 *     Adds an Timer to the timer_table.
 *
 * inputs     - name, callback function, arguments, frequency
 * outputs    - none
 */
void
timerAdd(const char *name, TMR *func, void *arg, time_t frequency)
{
    uint i;

    iassert(name != NULL);
    iassert(func != NULL);
    iassert(frequency != 0);

    ilog(L_DEBUG2, "timerAdd(): Adding %s", name);

    /* Find the first inactive table entry. */
    for (i = 0; i < MAX_TIMERS; i++)
    {
        if (timer_table[i].active == 0)
        {
            /* Set up the entry. */
            timer_table[i].name = name;
            timer_table[i].func = func;
            timer_table[i].arg = arg;
            timer_table[i].frequency = frequency;
            timer_table[i].when = (globals.currtime + frequency);
            timer_table[i].active = 1;

            if ((timer_table[i].when < timer_min) || (timer_min == -1))
                timer_min = timer_table[i].when;

            cnt.timer++;

            return;
        }
    }

    /* Looks like the table's full. */
    ilog(L_ERROR, "timerAdd(): Unable to add timer to table: %s", name);
}

/* timerAddOnce()
 *     Adds an Timer to the timer_table to be executed once only.
 *
 * inputs     - name, callback function, arguments, frequency
 * outputs    - none
 */
void
timerAddOnce(const char *name, TMR *func, void *arg, time_t frequency)
{
    uint i;

    iassert(name != NULL);
    iassert(func != NULL);
    iassert(frequency != 0);

    ilog(L_DEBUG2, "timerAddOnce(): Adding %s", name);

    /* Find the first inactive table entry. */
    for (i = 0; i < MAX_TIMERS; i++)
    {
        if (timer_table[i].active == 0)
        {
            /* Set up the entry. */
            timer_table[i].name = name;
            timer_table[i].func = func;
            timer_table[i].arg = arg;
            timer_table[i].frequency = 0;
            timer_table[i].when = (globals.currtime + frequency);
            timer_table[i].active = 1;

            if ((timer_table[i].when < timer_min) || (timer_min == -1))
                timer_min = timer_table[i].when;

            cnt.timer++;

            return;
        }
    }

    /* Looks like the table's full. */
    ilog(L_ERROR, "timerAdd(): Unable to add timer to table: %s", name);
}

/* timerFind()
 *     Finds a Timer in the timer_table by "func" and "arg".
 *
 * inputs     - callback function, argument
 * outputs    - index to the Timer in the timer_table, or -1 on not found
 */
int
timerFind(TMR *func, void *arg)
{
    uint i;

    iassert(func != NULL);

    for (i = 0; i < MAX_TIMERS; i++)
    {
        if ((timer_table[i].func == func) && (timer_table[i].arg == arg) &&
            (timer_table[i].active == 1))
            return i;
    }

    return -1;
}

/* timerDelete()
 *     Deletes a Timer from the timer_table.
 *
 * inputs     - callback function, argument
 * outputs    - 1 on success, 0 on not found
 */
uchar
timerDelete(TMR *func, void *arg)
{
    int i;

    iassert(func != NULL);

    i = timerFind(func, arg);

    if (i == -1)
        return 0;

    timer_table[i].name = NULL;
    timer_table[i].func = NULL;
    timer_table[i].arg = NULL;
    timer_table[i].active = 0;

    cnt.timer--;

    return 1;
}

/* timerNextRun()
 *     Checks to see when timerRun() needs to be called.
 *
 * inputs     - none;
 * outputs    - time when timerRun() needs to be called
 */
time_t
timerNextRun(void)
{
    int i;

    if (timer_min == -1)
    {
        for (i = 0; i < MAX_TIMERS; i++)
        {
            if (timer_table[i].active &&
                ((timer_table[i].when < timer_min) || (timer_min == -1)))
                timer_min = timer_table[i].when;
        }
    }

    return timer_min;
}

/* timerRun()
 *     Runs all pending Timers.
 *
 * inputs     - none
 * outputs    - none
 */
void
timerRun(void)
{
    uint i;

    for (i = 0; i < MAX_TIMERS; i++)
    {
        if ((timer_table[i].active == 1) &&
            (timer_table[i].when <= globals.currtime))
        {
            globals.last_timer = timer_table[i].name;
            timer_table[i].func(timer_table[i].arg);
            timer_min = -1;

            /* Check to see if it's scheduled more than once. */
            if (timer_table[i].frequency != 0)
                timer_table[i].when =
                    (globals.currtime + timer_table[i].frequency);
            else
            {
                timer_table[i].name = NULL;
                timer_table[i].func = NULL;
                timer_table[i].arg = NULL;
                timer_table[i].active = 0;

                cnt.timer--;
            }
        }
    }
}
