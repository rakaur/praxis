/*  praxis: services for TSora IRC networks.
 *  src/io.c: Network read/write loop.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "balloc.h"
#include "channel.h"
#include "events.h"
#include "event.h"
#include "connection.h"
#include "ilog.h"
#include "imem.h"
#include "net.h"
#include "server.h"
#include "send.h"
#include "sendq.h"
#include "timer.h"
#include "uplink.h"
#include "user.h"
#include "io.h"

static fd_set readfds;
static fd_set writefds;

/* sockRead()
 *     Reads incoming data.
 *
 * inputs     - Connection
 * outputs    - what was read
 */
static char
sockRead(Connection *connection_p)
{
    static char message[BUFSIZE + 1];
    static char *c = message;
    char rc;
    int n;
    static uchar data_in_buffer = 0;

    if (data_in_buffer == 0)
    {
        n = read(connection_p->fd, message, BUFSIZE);

        if (n <= 0)
        {
            connectionDead(connection_p);
            return -1;
        }

        message[n] = '\0';
        c = message;
        data_in_buffer = 1;
    }

    if (*c == '\0')
    {
        data_in_buffer = 0;
        message[0] = '\0';
        return *c;
    }

    rc = *c;
    c++;

    return rc;
}

/* evSendQReady()
 *     Flushes queued data.
 *
 * inputs     - Connection
 * outputs    - 1 on success or 0 on failure
 */
static uchar
evSendQReady(void *arg)
{
    Connection *connection_p = arg;

    iassert(connection_p != NULL);

    if (sendqFlush(connection_p) == 0)
        connectionDead(connection_p);

    return 1;
}

/* evConnectReturn()
 *     Finialises connect()'d sockets.
 *
 * inputs     - Connection
 * outputs    - 1 on success or 0 on failure
 */
static uchar
evConnectReturn(void *arg)
{
    Connection *connection_p = arg;
    struct sockaddr_in *sa;
    socklen_t salen = sizeof(struct sockaddr_in);
    int ret;

    iassert(connection_p != NULL);

    sa = ballocHeapAlloc(sa_heap);
    ret = getpeername(connection_p->fd, (struct sockaddr *)sa, &salen);

    if (ret == -1)
    {
        connectionDead(connection_p);
        return 0;
    }

    connection_p->flags &= ~CF_CONNECTING;
    connection_p->flags |= CF_CONNECTED;
    connection_p->sa = sa;

    if (CF_IS_UPLINK(connection_p))
        eventPost(E_CONNECTED, connection_p);

    else if (CF_IS_DCCOUT(connection_p))
        eventPost(E_DCCOUT, connection_p);

    else
    {
        ilog(L_DEBUG2, "evConnectReturn(): Connected something that's not an "
             "Uplink or a DCCOUT: %s (fd: %d)",
             connection_p->name, connection_p->fd);
    }

    return 1;
}

/* evConnected()
 *     Logs into the IRC uplink.
 *
 * inputs     - Connection
 * outputs    - 1 on success or 0 on failure
 */
static uchar
evConnected(void *arg)
{
    Connection *connection_p = arg;
    uchar ret;

    iassert(connection_p != NULL);

    ret = sendIRC(connection_p, "PASS %s TS 6 :%s", curr_uplink->pass, me.sid);

    if (ret == 0)
    {
        ilog(L_INFO, "evConnectReturn(): Connection to %s (fd: %d) failed",
             connection_p->name, connection_p->fd);

        return 0;
    }

    sendIRC(connection_p, "CAPAB :QS KLN UNKLN");
    sendIRC(connection_p, "SERVER %s 1 :%s", me.name, me.desc);
    sendIRC(connection_p, "SVINFO 6 3 0 :%d", globals.currtime);

    ilog(L_INFO, "evConnectReturn(): Connection with %s (fd: %d) "
         "established", connection_p->name, connection_p->fd);

    connection_p->last_recv_on = globals.currtime;

    return 1;
}

/* ioReconnect()
 *     Reconnects to the IRC server.
 *
 * inputs     - Timer argument
 * outputs    - none
 */
static void
ioReconnect(void *arg)
{
    Connection *connection_p;

    connection_p = uplinkConnect();

    if (connection_p == NULL)
        timerAddOnce("ioReconnect", ioReconnect, NULL, settings.reconnect_time);
}

/* evDisconnected()
 *     Sets a timer to reconnect to the IRC server.
 *
 * inputs     - Event argument
 * outputs    - 1 on success or 0 on failure
 */
static uchar
evDisconnected(void *arg)
{
    serverFlush();
    userFlush();
    channelFlush();

    timerAddOnce("ioReconnect", ioReconnect, NULL, settings.reconnect_time);

    return 1;
}

static uchar
evListenReturn(void *arg)
{
    Connection *connection_p = arg;
    int accept_sock;
    struct sockaddr_in *incoming_addr;
    int addrlen = sizeof(struct sockaddr_in);

    iassert(connection_p != NULL);

    incoming_addr = ballocHeapAlloc(sa_heap);

    accept_sock = accept(connection_p->fd, (struct sockaddr *)incoming_addr,
                         (socklen_t *)&addrlen);

    if (accept_sock < 0)
    {
        if (errno == EAGAIN)
            return 1;

        connectionDead(connection_p);

        ballocHeapFree(sa_heap, incoming_addr);

        return 0;
    }

    if (accept_sock > globals.max_fd)
        globals.max_fd = accept_sock;

    /* Since the socket has been accept()'d, we have a new fd to use.
     * Close the listening one and swap it with this one.
     */
    shutdown(connection_p->fd, SHUT_RDWR);
    close(connection_p->fd);

    connection_p->fd = accept_sock;
    connection_p->sa = incoming_addr;
    connection_p->flags &= ~CF_LISTENING;
    connection_p->flags |= CF_CONNECTED;

    if (CF_IS_DCCIN(connection_p))
        eventPost(E_DCCIN, connection_p);
    else
    {
        ilog(L_DEBUG2, "evListenReturn(): Accepted something that's not a "
             "DCCIN: %s (fd: %d)", connection_p->name, connection_p->fd);
    }

    return 1;
}

/* evReadReady()
 *     read()'s data on a socket.
 *
 * inputs     - Connection
 * outputs    - 1 on success or 0 on failure
 */
static uchar
evReadReady(void *arg)
{
    Connection *connection_p = arg;
    char message[BUFSIZE + 1];
    int i = 0, c;
    uchar posted = 0;

    iassert(connection_p != NULL);

    if (CF_IS_DEAD(connection_p))
    {
        ilog(L_ERROR, "evReadReady(): Got dead Connection: %s (fd: %d)",
             connection_p->name, connection_p->fd);

        return 0;
    }

    if (connection_p->hbuf[0] != '\0')
    {
        strlcpy(message, connection_p->hbuf, (BUFSIZE + 1));
        i = strlen(message);
        message[i] = '\0';
        connection_p->hbuf[0] = '\0';
    }

    while (1)
    {
        c = sockRead(connection_p);

        connection_p->last_recv_on = globals.currtime;
        connection_p->pinged = 0;

        if (c < 0)
            return 0;

        if (c == '\0')
        {
            if (i > 0)
            {
                message[i] = '\0';
                strlcpy(connection_p->hbuf, message, (BUFSIZE + 1));
            }

            break;
        }

        if ((c == '\r') || (c == '\n'))
        {
            if (i > 0)
            {
                message[i] = '\0';

                cnt.inb += strlen(message);

                dlinkAddTailAlloc(istrdup(message), &connection_p->recvq);

                if (posted == 0)
                {
                    eventPost(E_PARSE, connection_p);
                    posted = 1;
                }

                i = 0;
            }

            continue;
        }

        message[i] = c;
        i++;
    }

    return 1;
}

/* ioTimeouts()
 *     Checks all Connections for timeouts and such.
 *
 * inputs     - Timer argument
 * outputs    - none
 */
static void
ioTimeouts(void *arg)
{
    Connection *connection_p;
    DLinkNode *node_p;

    DLINK_FOREACH(node_p, connection_list.head)
    {
        connection_p = node_p->data;

        if (CF_IS_CONNECTING(connection_p))
        {
            if ((connection_p->first_data_on + 30) <= globals.currtime)
            {
                ilog(L_INFO, "ioTimeouts(): Connection timed out for %s "
                     "(fd: %d): no data in %d seconds",
                     connection_p->name, connection_p->fd,
                     (globals.currtime - connection_p->first_data_on));

                connectionDead(connection_p);
            }
        }
        else if (CF_IS_LISTENING(connection_p))
        {
            if ((connection_p->first_data_on + 30) <= globals.currtime)
            {
                ilog(L_INFO, "ioTimeouts(): Listen timed out for %s "
                     "(fd: %d): no data in %d seconds",
                     connection_p->name, connection_p->fd,
                     (globals.currtime - connection_p->first_data_on));

                connectionDead(connection_p);
            }
        }
        else if ((connection_p->pinged == 1) &&
                 ((connection_p->last_recv_on + settings.ping_time) <=
                  globals.currtime))
        {
            ilog(L_INFO, "ioTimeouts(): Ping timeout for %s (fd: %d): no "
                 "response in %d seconds",
                 connection_p->name, connection_p->fd,
                 (globals.currtime - connection_p->last_recv_on));

            connectionDead(connection_p);
        }
        else
        {
            if ((connection_p->pinged == 0) && (CF_IS_UPLINK(connection_p))
                && ((connection_p->last_recv_on + 300) <= globals.currtime))
            {
                sendIRC(connection_p, "PING :%s",
                        (curr_uplink->ts_version == 6) ? me.sid : me.name);

                connection_p->pinged = 1;
            }
        }
    }
}


/* ioInit()
 *     Initialises io events.
 *
 * inputs     - none
 * outputs    - none
 */
void
ioInit(void)
{
    eventAddSpecial(E_CONNECTRETURN, 1, evConnectReturn);
    eventAddSpecial(E_CONNECTED, 1, evConnected);
    eventAddSpecial(E_DISCONNECTED, 2, evDisconnected);
    eventAddSpecial(E_SENDQREADY, 2, evSendQReady);
    eventAddSpecial(E_LISTENRETURN, 1, evListenReturn);
    eventAddSpecial(E_READREADY, 1, evReadReady);

    timerAdd("ioTimeouts", ioTimeouts, NULL, 60);
}

/* io()
 *     The main network read/write loop.
 *     Events, timers, and a ton of other things happen here.
 *
 * inputs     - none
 * outputs    - none
 */
void
io(void)
{
    Connection *connection_p;
    DLinkNode *node_p;
    struct timeval timeout;
    time_t delay;
    int ret;

    while (!(globals.run_flags & (RF_SHUTDOWN | RF_RESTART)))
    {
        globals.currtime = time(NULL);

        /* All dead connections are safe to remove. */
        DLINK_FOREACH(node_p, dying_list.head)
        {
            eventPost(E_CONNDEAD, node_p);
        }

        /* Clear the event queue.
         * We loop until it's clear in case events add more events.
         */
        while (eventShouldRun() == 1)
            eventRun();

        /* Do the timers. */
        delay = timerNextRun();

        if (delay <= globals.currtime)
            timerRun();

        /* Ready the select() stuff. */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        timeout.tv_usec = 0;

        /* Here's the deal:
         * We only exit select() for one of three reasons:
         *
         * 1. select() returns because of data waiting to be read.
         * 2. select() returns because of a socket being ready to write.
         * 3. select() returns because it timed out.
         *
         * 1 and 2 will happen no matter what.  When we come back around, the
         * event loop is ran so that takes care of that.
         *
         * 3 we need for timers.  We don't want to waste CPU, so we can set
         * the timeout for the number of seconds before the next time any timers
         * need to go off.  Make sense?
         *
         * The only other reason to timeout sooner is so we can check dead
         * connections and connection timeouts.  Neither are really
         * important enough to timeout any sooner, though.
         */
        if (delay > globals.currtime)
            timeout.tv_sec = (delay - globals.currtime);
        else
            timeout.tv_sec = 1;

        /* Now go through and set up the fd_sets. */
        DLINK_FOREACH(node_p, connection_list.head)
        {
            connection_p = node_p->data;

            /* Any call to connect() will be waiting to write. */
            if (CF_IS_CONNECTING(connection_p))
                FD_SET(connection_p->fd, &writefds);

            /* Any call to listen() will be waiting to read. */
            else if (CF_IS_LISTENING(connection_p))
                FD_SET(connection_p->fd, &readfds);

            /* Connected sockets will be waiting to read. */
            else
                FD_SET(connection_p->fd, &readfds);

            /* Connected sockets with queued data will be waiting to write. */
            if (dlinkLength(&connection_p->sendq) > 0)
                FD_SET(connection_p->fd, &writefds);
        }

        /* Now we have everything ready for select(). */
        ret = select((globals.max_fd + 1), &readfds, &writefds, NULL, &timeout);

        if (ret > 0)
        {
            /* Go through each one to see what we need to do. */
            DLINK_FOREACH(node_p, connection_list.head)
            {
                connection_p = node_p->data;

                if (FD_ISSET(connection_p->fd, &writefds))
                {
                    if (CF_IS_CONNECTING(connection_p))
                    {
                        eventPost(E_CONNECTRETURN, connection_p);
                        continue;
                    }
                    else if (dlinkLength(&connection_p->sendq) > 0)
                    {
                        eventPost(E_SENDQREADY, connection_p);
                        continue;
                    }
                }
                if (FD_ISSET(connection_p->fd, &readfds))
                {
                    if (CF_IS_LISTENING(connection_p))
                    {
                        eventPost(E_LISTENRETURN, connection_p);
                        continue;
                    }
                    else
                        eventPost(E_READREADY, connection_p);
                }
            }
        }
        else if (ret < 0)
        {
            static int local_error;

            /* Some form of select() error occurred.  Sometimes we can ignore
             * and others we need to do something with: XXX
             */
            if (errno == EINTR)
                continue;

            local_error = errno;
            eventPost(E_SELECTERROR, &local_error);
        }
        else
        {
            /* This is just select() timing out. */
        }
    }
}
