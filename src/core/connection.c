/*  praxis: services for TSora IRC networks.
 *  src/connection.c: Maintains the connection list.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "balloc.h"
#include "events.h"
#include "event.h"
#include "ilog.h"
#include "connection.h"
#include "sendq.h"
#include "uplink.h"

static uchar evConnDead(void *);

static Heap *connection_heap;

/* connectionInit()
 *     Initialises the connection list.
 *
 * inputs     - none
 * outputs    - none
 */
void
connectionInit(void)
{
    connection_heap = ballocHeapCreate(sizeof(Connection), CONN_HEAP_SIZE);
    sa_heap = ballocHeapCreate(sizeof(struct sockaddr_in), CONN_HEAP_SIZE);
    memset(&connection_list, '\0', sizeof(connection_list));
    memset(&dying_list, '\0', sizeof(dying_list));

    if ((connection_heap == NULL) || (sa_heap == NULL))
    {
        ilog(L_INFO, "connectionInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }

    eventAddSpecial(E_CONNDEAD, 2, evConnDead);
}

/* connectionAdd()
 *     Adds a Connection to the connection_list.
 *
 * inputs     - name, fd, flags
 * outputs    - pointer to Connection or NULL on failure
 */
Connection *
connectionAdd(const char *name, int fd, uint flags)
{
    Connection *connection_p;

    iassert(fd >= 0);
    iassert(name != NULL);

    ilog(L_DEBUG2, "connectionAdd(): Adding %s (fd: %d)", name, fd);

    connection_p = ballocHeapAlloc(connection_heap);

    strlcpy(connection_p->name, name, HOSTLEN);

    connection_p->fd = fd;
    connection_p->flags |= flags;
    connection_p->first_data_on = globals.currtime;
    connection_p->last_recv_on = 0;
    connection_p->pinged = 0;
    connection_p->sa = NULL;

    dlinkAddAlloc(connection_p, &connection_list);

    cnt.connection++;

    return connection_p;
}

/* connectionFind()
 *     Finds a Connection in the connection_list by "fd".
 *
 * inputs     - fd
 * outputs    - pointer to Connection or NULL on failure
 */
Connection *
connectionFind(int fd)
{
    Connection *connection_p;
    DLinkNode *node_p;

    iassert(fd > 0);

    DLINK_FOREACH(node_p, connection_list.head)
    {
        connection_p = node_p->data;

        if (connection_p->fd == fd)
            return connection_p;
    }

    return NULL;
}

/* connectionDelete()
 *     Removes a Connection from the connection_list.
 *
 * inputs     - Connection
 * outputs    - 1 on success or 0 on failure
 */
uchar
connectionDelete(Connection *connection_p)
{
    uchar ret;

    iassert(connection_p != NULL);

    ret = dlinkFindDestroy(connection_p, &connection_list);

    if (ret == 0)
    {
        ilog(L_DEBUG2, "connectionDelete(): Cannot find Connection: %s "
             "(fd: %d)", connection_p->name, connection_p->fd);

        return 0;
    }

    if (connection_p->sa != NULL)
        ballocHeapFree(sa_heap, connection_p->sa);

    ballocHeapFree(connection_heap, connection_p);

    cnt.connection--;

    return 1;
}

/* connectionDead()
 *     Marks a Connection as dead in the dying_list.
 *
 * inputs     - Connection
 * outputs    - none
 */
void
connectionDead(Connection *connection_p)
{
    DLinkNode *node_p;

    iassert(connection_p != NULL);

    ilog(L_DEBUG2, "connectionDead(): Marking %s (fd: %d) as dead",
         connection_p->name, connection_p->fd);

    connection_p->flags = CF_DEAD;
    node_p = dlinkFind(connection_p, &connection_list);

    if (node_p == NULL)
    {
        ilog(L_DEBUG2, "connectionDead(): Cannot find Connection: %s "
             "(fd: %d)", connection_p->name, connection_p->fd);

        return;
    }

    dlinkNodeMove(node_p, &connection_list, &dying_list);

    if (connection_p == curr_uplink->connection_p)
        eventPost(E_DISCONNECTED, NULL);
}

/* evConnDead()
 *     Called from the event loop when a Connection gets marked as dead.
 *
 * inputs     - Connection
 * outputs    - 1 on success or 0 on failure
 */
static uchar
evConnDead(void *arg)
{
    SendQ *sendq_p;
    DLinkNode *node_p, *tnode_p, *anode_p = arg;
    Connection *connection_p = anode_p->data;

    iassert(connection_p != NULL);

    ilog(L_DEBUG2, "evConnDead(): Destroying Connection %s (fd: %d)",
         connection_p->name, connection_p->fd);

    shutdown(connection_p->fd, SHUT_RDWR);
    close(connection_p->fd);

    /* Clear the SendQ. */
    DLINK_FOREACH_SAFE(node_p, tnode_p, connection_p->sendq.head)
    {
        sendq_p = node_p->data;

        dlinkDestroy(node_p, &connection_p->sendq);

        ballocHeapFree(sendq_heap, sendq_p);
    }

    dlinkDestroy(anode_p, &dying_list);

    ballocHeapFree(connection_heap, connection_p);

    cnt.connection--;

    return 1;
}
