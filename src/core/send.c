/*  praxis: services for TSora IRC networks.
 *  src/send.c: Send data to sockets.
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
#include "dlink.h"
#include "ilog.h"
#include "sendq.h"
#include "uplink.h"
#include "send.h"

/* sendIRC()
 *     Writes data to an IRC server, appending \r\n.
 *
 * inputs     - Connection, format string, parameters
 * outputs    - 1 on success or 0 on failure
 */
uchar
sendIRC(Connection *connection_p, char *fmt, ...)
{
    va_list ap;
    char buf[BUFSIZE + 1];
    int len, n;

    iassert(connection_p != NULL);

    if (CF_IS_DEAD(connection_p))
    {
        ilog(L_DEBUG2, "sendIRC(): Got dead Connection: %s (fd: %d)",
             connection_p->name, connection_p->fd);

        return 1;
    }

    va_start(ap, fmt);
    vsnprintf(buf, (BUFSIZE + 1), fmt, ap);
    va_end(ap);

    ilog(L_DEBUG1, "<- %s", buf);

    strlcat(buf, "\r\n", (BUFSIZE + 1));

    len = strlen(buf);

    cnt.outb += len;

    n = sendqFlush(connection_p);

    if (n == 0)
    {
        if (errno != EAGAIN)
        {
            connectionDead(connection_p);
            return 0;
        }
        else
        {
            sendqAdd(connection_p, buf, len, 0);
            return 1;
        }
    }
    else if (n == 2)
    {
        sendqAdd(connection_p, buf, len, 0);
        return 1;
    }

    n = write(connection_p->fd, buf, len);

    if (n == -1)
    {
        if (errno == EAGAIN)
        {
            connectionDead(connection_p);
            return 0;
        }
        else
            sendqAdd(connection_p, buf, len, 0);
    }
    else if (n != len)
        sendqAdd(connection_p, buf, len, n);

    return 1;
}
