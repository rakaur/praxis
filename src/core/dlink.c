/*  praxis: services for TSora IRC networks.
 *  src/dlink.c: Linked list routines.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *  Copyright (c) 2002-2004 ircd-ratbox development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "balloc.h"
#include "ilog.h"

static Heap *dlink_heap;

/* dlinkInit()
 *     Initialises the linked list system.
 *
 * inputs     - none
 * outputs    - none
 */
void
dlinkInit(void)
{
    dlink_heap = ballocHeapCreate(sizeof(DLinkNode), DLINK_HEAP_SIZE);

    if (dlink_heap == NULL)
    {
        ilog(L_INFO, "dlinkInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }
}

/* NOTE: Nearly *all* of these are *heavily* used.
 * Multple calls to each are made on each network state change.
 * Therefore, I have inlined them in hopes of speed.
 */

/* dlinkNodeCreate()
 *     Allocates a new node for use in lists.
 *
 * inputs     - none
 * outputs    - the new node
 */
inline DLinkNode *
dlinkNodeCreate(void)
{
    DLinkNode *node_p = ballocHeapAlloc(dlink_heap);

    cnt.node++;

    return node_p;
}

/* dlinkNodeFree()
 *     Returns an unused node to the heap.
 *
 * inputs     - node to free
 * outputs    - none
 */
inline void
dlinkNodeFree(DLinkNode *node_p)
{
    iassert(node_p != NULL);

    ballocHeapFree(dlink_heap, node_p);

    cnt.node--;
}

/* dlinkAdd()
 *     Adds a node to the head of a list.
 *
 * inputs     - data to associate with node, node, list
 * outputs    - none
 */
inline void
dlinkAdd(void *data, DLinkNode *node_p, DLinkList *list_p)
{
    iassert(data != NULL);
    iassert(node_p != NULL);
    iassert(list_p != NULL);

    node_p->data = data;
    node_p->next = list_p->head;

    if (list_p->head != NULL)
        list_p->head->prev = node_p;
    else if (list_p->tail == NULL)
        list_p->tail = node_p;

    list_p->head = node_p;
    list_p->length++;
}

/* dlinkAddTail()
 *     Adds a node to the tail of a list.
 *
 * inputs     - data to associate with node, node, list
 * outputs    - none
 */
inline void
dlinkAddTail(void *data, DLinkNode *node_p, DLinkList *list_p)
{
    iassert(data != NULL);
    iassert(node_p != NULL);
    iassert(list_p != NULL);

    node_p->data = data;
    node_p->next = NULL;
    node_p->prev = list_p->tail;

    if (list_p->tail != NULL)
        list_p->tail->next = node_p;
    else if (list_p->head == NULL)
        list_p->head = node_p;

    list_p->tail = node_p;
    list_p->length++;
}

/* dlinkFind()
 *     Finds a node in a list by "data"
 *
 * inputs     - list, data
 * outputs    - node, or NULL on not found
 */
inline DLinkNode *
dlinkFind(void *data, DLinkList *list_p)
{
    DLinkNode *node_p;

    iassert(list_p != NULL);
    iassert(data != NULL);

    DLINK_FOREACH(node_p, list_p->head)
    {
        if (node_p->data == data)
            return node_p;
    }

    return NULL;
}

/* dlinkDelete()
 *     Removes a node from a list.
 *
 * inputs     - node, list
 * outputs    - none
 */
inline void
dlinkDelete(DLinkNode *node_p, DLinkList *list_p)
{
    iassert(node_p != NULL);
    iassert(list_p != NULL);

    if (node_p->next != NULL)
        node_p->next->prev = node_p->prev;
    else
        list_p->tail = node_p->prev;

    if (node_p->prev != NULL)
        node_p->prev->next = node_p->next;
    else
        list_p->head = node_p->next;

    node_p->next = node_p->prev = NULL;
    list_p->length--;
}

/* dlinkFindDelete()
 *     Finds a node in a list and removes it.
 *
 * inputs     - list, data
 * outputs    - unused node
 */
inline DLinkNode *
dlinkFindDelete(void *data, DLinkList *list_p)
{
    DLinkNode *node_p;

    iassert(data != NULL);
    iassert(list_p != NULL);

    DLINK_FOREACH(node_p, list_p->head)
    {
        if (node_p->data != data)
            continue;

        if (node_p->next)
            node_p->next->prev = node_p->prev;
        else
            list_p->tail = node_p->prev;

        if (node_p->prev)
            node_p->prev->next = node_p->next;
        else
            list_p->head = node_p->next;

        node_p->next = node_p->prev = NULL;
        list_p->length--;

        return node_p;
    }

    return NULL;
}

/* dlinkFindDestroy()
 *     Finds a node in a list, removes it, and frees it.
 *
 * inputs     - list, data
 * outputs    - 0 on fail, 1 on success
 */
inline uchar
dlinkFindDestroy(void *data, DLinkList *list_p)
{
    void *ptr;

    iassert(list_p != NULL);
    iassert(data != NULL);

    ptr = dlinkFindDelete(data, list_p);

    if (ptr != NULL)
    {
        dlinkNodeFree(ptr);
        return 1;
    }

    return 0;
}

/* dlinkNodeMove()
 *     Moves a node and data from one list to another.
 *
 * inputs     - node, old list, new list
 * outputs    - none
 */
inline void
dlinkNodeMove(DLinkNode *node_p, DLinkList *olist_p, DLinkList *nlist_p)
{
    iassert(node_p != NULL);
    iassert(olist_p != NULL);
    iassert(nlist_p != NULL);

    if (node_p->next)
        node_p->next->prev = node_p->prev;
    else
        olist_p->tail = node_p->prev;

    if (node_p->prev)
        node_p->prev->next = node_p->next;
    else
        olist_p->head = node_p->next;

    node_p->prev = NULL;
    node_p->next = nlist_p->head;

    if (nlist_p->head != NULL)
        nlist_p->head->prev = node_p;
    else if (nlist_p->tail == NULL)
        nlist_p->tail = node_p;

    nlist_p->head = node_p;

    olist_p->length--;
    nlist_p->length++;
}
