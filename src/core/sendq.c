/*  praxis: services for TSora IRC networks.
 *  src/sendq.c: Manages SendQs.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "balloc.h"
#include "connection.h"
#include "ilog.h"
#include "sendq.h"

Heap *sendq_heap;

/* sendqInit()
 *     Initialises the sendq system.
 *
 * inputs     - none
 * outputs    - none
 */
void
sendqInit(void)
{
    sendq_heap = ballocHeapCreate(sizeof(SendQ), SENDQ_HEAP_SIZE);

    if (sendq_heap == NULL)
    {
        ilog(L_INFO, "sendqInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }
}

/* sendqAdd()
 *     Queues data to be written in a SendQ.
 *
 * inputs     - Connection, data, length, position
 * outputs    - none
 */
void
sendqAdd(Connection *connection_p, const char *buf, int len, int pos)
{
    SendQ *sendq_p = ballocHeapAlloc(sendq_heap);

    iassert(connection_p != NULL);
    iassert(buf != NULL);
    iassert(len != 0);
    iassert(pos != len);

    ilog(L_DEBUG2, "sendqAdd(): SendQ triggered for %s (fd: %d)",
         connection_p->name, connection_p->fd);

    strlcpy(sendq_p->buf, buf, BUFSIZE);
    sendq_p->len = (len - pos);
    sendq_p->pos = pos;

    dlinkAddTailAlloc(sendq_p, &connection_p->sendq);

    cnt.sendq++;
}

/* sendqFlush()
 *     Flushes a Connection's SendQ.
 *
 * inputs     - Connection
 * outputs    - 1 on success, 2 on partial write, or 0 on failure
 */
uchar
sendqFlush(Connection *connection_p)
{
    DLinkNode *node_p, *tnode_p;
    SendQ *sendq_p;
    int n;

    iassert(connection_p != NULL);

    DLINK_FOREACH_SAFE(node_p, tnode_p, connection_p->sendq.head)
    {
        sendq_p = node_p->data;

        n = write(connection_p->fd, (sendq_p->buf + sendq_p->pos),
                  sendq_p->len);

        if (n == -1)
        {
            if (errno != EAGAIN)
                return 0;

            return 2;
        }

        /* If we wrote it all, destroy the SendQ. */
        if (n == sendq_p->len)
        {
            dlinkDestroy(node_p, &connection_p->sendq);
            ballocHeapFree(sendq_heap, sendq_p);

            cnt.sendq--;
        }
        else
        {
            sendq_p->pos += n;
            sendq_p->len -= n;

            return 0;
        }
    }

    return 1;
}
