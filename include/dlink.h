/*  praxis: services for TSora IRC networks.
 *  include/dlink.h: Contains forward declarations for dlink.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_dlink_h
#define INCLUDED_dlink_h

typedef struct DLinkNode DLinkNode;
typedef struct DLinkList DLinkList;

struct DLinkNode
{
    void *data;
    DLinkNode *prev, *next;
};

struct DLinkList
{
    DLinkNode *head, *tail;
    ulong length;
};

#define DLINK_FOREACH(pos, head) for (pos = (head); pos != NULL; pos = pos->next)

#define DLINK_FOREACH_SAFE(pos, n, head) for (pos = (head), n = pos ? pos->next : NULL; pos != NULL; pos = n, n = pos ? pos->next : NULL)

#define dlinkLength(list) (list)->length
#define dlinkAddAlloc(data, list) dlinkAdd(data, dlinkNodeCreate(), list)
#define dlinkAddTailAlloc(data, list) dlinkAddTail(data, dlinkNodeCreate(), list)

#define dlinkDestroy(node, list) do { dlinkDelete(node, list); dlinkNodeFree(node); } while (0);

void dlinkInit(void);
DLinkNode *dlinkNodeCreate(void);
void dlinkNodeFree(DLinkNode *);
void dlinkAdd(void *, DLinkNode *, DLinkList *);
void dlinkAddTail(void *, DLinkNode *, DLinkList *);
DLinkNode *dlinkFind(void *, DLinkList *);
void dlinkDelete(DLinkNode *, DLinkList *);
DLinkNode *dlinkFindDelete(void *, DLinkList *);
uchar dlinkFindDestroy(void *, DLinkList *);
void dlinkNodeMove(DLinkNode *, DLinkList *, DLinkList *);

#endif /* INCLUDED_dlink_h */
