/*  praxis: services for TSora IRC networks.
 *  src/event.c: The event loop routines.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "balloc.h"
#include "ilog.h"
#include "events.h"
#include "event.h"

static Heap *eventq_heap;

/* eventInit()
 *     Initialises the event system.
 *
 * inputs     - none
 * outputs    - none
 */
void
eventInit(void)
{
    eventq_heap = ballocHeapCreate(sizeof(EventQ), EVENTQ_HEAP_SIZE);

    memset(&event_table, '\0', (sizeof(Event) * E_LAST));
    memset(&event_queue, '\0', sizeof(event_queue));

    if (eventq_heap == NULL)
    {
        ilog(L_INFO, "eventInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }
}

/* eventAdd()
 *     Adds a function to be called when an event happens.
 *
 * inputs     - E_event, pointer to function
 * outputs    - none
 */
void
eventAdd(uint event, uchar (*func) ())
{
    iassert(event >= 0);
    iassert(event < E_LAST);
    iassert(func != NULL);

    ilog(L_DEBUG2, "eventAdd(): Registering function for event: %d", event);

    dlinkAddTailAlloc(func, &event_table[event].func_list);
}

/* eventAddSpecial()
 *     Adds a function to be called first or last when an event happens.
 *
 * inputs     - E_event, 1 for first 2 for last, pointer to function
 * outputs    - 1 on success or 0 on failure
 */
uchar
eventAddSpecial(uint event, uchar type, uchar (*func) ())
{
    iassert(event >= 0);
    iassert(event < E_LAST);
    iassert(type == 1 || type == 2);
    iassert(func != NULL);

    if (type == 1)
    {
        if (event_table[event].first_func != NULL)
            return 0;

        ilog(L_DEBUG2, "eventAddSpecial(): Registering first function for "
             "event: %d", event);

        event_table[event].first_func = func;
    }
    else if (type == 2)
    {
        if (event_table[event].last_func != NULL)
            return 0;

        ilog(L_DEBUG2, "eventAddSpecial(): Registering last function for "
             "event: %d", event);

        event_table[event].last_func = func;
    }
    else
        return 0;

    return 1;
}

/* eventDelete()
 *     Removes a function to be called when an event happens.
 *
 * inputs     - E_event, pointer to function
 * outputs    - 1 on success or 0 on failure
 */
uchar
eventDelete(uint event, uchar (*func) ())
{
    uchar ret;

    iassert(event >= 0);
    iassert(event < E_LAST);
    iassert(func != NULL);

    ilog(L_DEBUG2, "eventDelete(): Unregistering function for event: %d",
         event);

    ret = dlinkFindDestroy(func, &event_table[event].func_list);

    if (ret == 0)
    {
        ilog(L_ERROR, "eventDelete(): dlinkFindDestroy() failed.");
        return 0;
    }

    return 1;
}

/* eventDeleteSpecial()
 *     Removes a function to be called first or last when an event happens.
 *
 * inputs     - E_event, 1 for first 2 for last, pointer to function
 * outputs    - 1 on success or 0 on failure
 */
uchar
eventDeleteSpecial(uint event, uchar type, uchar (*func) ())
{
    iassert(event >= 0);
    iassert(event < E_LAST);
    iassert(type == 1 || type == 2);
    iassert(func != NULL);

    if (type == 1)
    {
        if (event_table[event].first_func != func)
            return 0;

        ilog(L_DEBUG2, "eventDeleteSpecial(): Unregistering first function for "
             "event: %d", event);

        event_table[event].first_func = NULL;
    }
    else if (type == 2)
    {
        if (event_table[event].last_func != func)
            return 0;

        ilog(L_DEBUG2, "eventDeleteSpecial(): Unregistering last function for "
             "event: %d", event);

        event_table[event].last_func = NULL;
    }
    else
        return 0;

    return 1;
}

/* eventPost()
 *     Post an event to the event_queue that has happened.
 *
 * inputs     - E_event, argument
 * outputs    - none
 */
void
eventPost(uint event, void *arg)
{
    EventQ *eventq_p;

    iassert(event >= 0);
    iassert(event < E_LAST);

    /* Check to see if this Event is being handled. */
    if ((event_table[event].first_func == NULL) &&
        (dlinkLength(&event_table[event].func_list) == 0) &&
        (event_table[event].last_func == NULL))
        return;

    eventq_p = ballocHeapAlloc(eventq_heap);

    eventq_p->event = event;
    eventq_p->arg = arg;

    dlinkAddTailAlloc(eventq_p, &event_queue);
}

/* eventShouldRun()
 *     Returns if we need to clear the queue or not.
 *
 * inputs     - none
 * outputs    - 1 for yes, or 0 for no
 */
uchar
eventShouldRun(void)
{
    if (dlinkLength(&event_queue) != 0)
        return 1;

    return 0;
}

/* eventRun()
 *     Runs all pending events.
 *
 * inputs     - none
 * outputs    - none
 */
void
eventRun(void)
{
    EventQ *eventq_p;
    DLinkNode *node_p, *tnode_p, *node2_p;
    uchar (*func) ();
    uchar ret, terminated = 0;

    /* Should we even bother? */
    if (dlinkLength(&event_queue) == 0)
        return;

    DLINK_FOREACH_SAFE(node_p, tnode_p, event_queue.head)
    {
        eventq_p = node_p->data;

        /* If there's a first function, call it. */
        if (event_table[eventq_p->event].first_func != NULL)
        {
            ret = event_table[eventq_p->event].first_func(eventq_p->arg);

            if (ret == 0)
            {
                ilog(L_ERROR, "eventRun(): first_func failure for event: %d.",
                     eventq_p->event);

                terminated = 1;
            }
        }

        /* If there are function list entries, call them. */
        DLINK_FOREACH(node2_p, event_table[eventq_p->event].func_list.head)
        {
            func = node2_p->data;

            if (terminated == 1)
                break;

            ret = func(eventq_p->arg);

            if (ret == 0)
            {
                ilog(L_ERROR, "eventRun(): func_list failure for event: %d.",
                     eventq_p->event);

                terminated = 1;
            }
        }

        /* If there's a last function, call it. */
        if (event_table[eventq_p->event].last_func != NULL)
        {
            ret = event_table[eventq_p->event].last_func(eventq_p->arg);

            if (ret == 0)
            {
                ilog(L_ERROR, "eventRun(): last_func failure for event: %d.",
                     eventq_p->event);
            }
        }

        /* Now destroy the event_queue entry. */
        dlinkDestroy(node_p, &event_queue);
        ballocHeapFree(eventq_heap, eventq_p);
    }
}
