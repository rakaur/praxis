/*  praxis: services for TSora IRC networks.
 *  src/net.c: Network socket code.
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
#include "net.h"

#ifdef HAVE_GETADDRINFO
/* gethostinfo()
 *     Stolen from FreeBSD's whois client.
 *
 * inputs     - host, port
 * outputs    - pointer to struct addrinfo or NULL on failure
 */
static struct addrinfo *
gethostinfo(const char *host, int port)
{
    struct addrinfo hints, *res;
    int error;
    char portbuf[6];

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portbuf, 6, "%d", port);

    error = getaddrinfo(host, portbuf, &hints, &res);

    if (error != 0)
    {
        ilog(L_INFO, "gethostinfo(): error on %s: %s", host,
             gai_strerror(error));

        return NULL;
    }

    return res;
}

/* netConnect()
 *     Connects a socket to a host on a port.
 *
 * inputs     - host, port, source
 * outputs    - pointer to Connection or NULL on failure
 */
Connection *
netConnect(const char *host, int port, const char *source)
{
    struct addrinfo *hostres, *res;
    struct addrinfo *bindres = NULL;
    int s = -1, optval, flags;

    iassert(host != NULL);
    iassert(port > 0);

    hostres = gethostinfo(host, port);

    if (hostres == NULL)
    {
        ilog(L_ERROR, "netConnect(): gethostinfo() failed.");
        return NULL;
    }

    /* If there's a source, look up the info on it. */
    if (source != NULL)
    {
        bindres = gethostinfo(source, 0);

        if (bindres == NULL)
        {
            ilog(L_ERROR, "netConnect(): gethostinfo() failed.");
            return NULL;
        }
    }

    for (res = hostres; res; res = res->ai_next)
    {
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        if (s < 0)
            continue;

        /* Update our fd tracking. */
        if (s > globals.max_fd)
            globals.max_fd = s;

        optval = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&optval,
                   sizeof(optval));

        /* If there's a source, bind to it. */
        if (bindres != NULL)
        {
            if (bind(s, bindres->ai_addr, bindres->ai_addrlen) < 0)
            {
                ilog(L_ERROR, "netConnect(): Cannot bind to %s", source);
                return NULL;
            }
        }

        /* Set the flags and use nonblocking sockets. */
        flags = fcntl(s, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(s, F_SETFL, flags);

        /* Now finally connect it. */
        connect(s, res->ai_addr, res->ai_addrlen);

        break;
    }

    freeaddrinfo(hostres);

    if (bindres != NULL)
        freeaddrinfo(bindres);

    if (res == NULL)
    {
        ilog(L_ERROR, "netConnect(): Cannot connect to host");
        return NULL;
    }

    return connectionAdd("IRC Uplink", s, CF_CONNECTING);
}
#else
/* netConnect()
 *     Connects a socket to a host on a port.
 *
 * inputs     - host, port, source
 * outputs    - pointer to Connection or NULL on failure
 */
Connection *
netConnect(const char *host, int port, const char *source)
{
    struct hostent *hp;
    struct sockaddr_in sa;
    struct in_addr *in;
    int s, optval, flags;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ilog(L_ERROR, "netConnect(): Cannot create socket");
        return NULL;
    }

    /* Update our fd tracking. */
    if (s > globals.max_fd)
        globals.max_fd = s;

    /* If there's a source, bind to it. */
    if (source != NULL)
    {
        memset(&sa, '\0', sizeof(sa));

        sa.sin_family = AF_INET;

        if ((hp = gethostbyname(source)) == NULL)
        {
            ilog(L_ERROR, "netConnect(): gethostbyname() failed: %s", source);
            close(s);
            return NULL;
        }

        in = (struct in_addr *)(hp->h_addr_list[0]);
        sa.sin_addr.s_addr = in->s_addr;
        sa.sin_port = 0;

        optval = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&optval,
                   sizeof(optval));

        if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        {
            ilog(L_ERROR, "netConnect(): Unabled to bind to %s", source);
            close(s);
            return NULL;
        }
    }

    if ((hp = gethostbyname(host)) == NULL)
    {
        ilog(L_ERROR, "netConnect(): gethostbyname failed: %s", host);
        close(s);
        return NULL;
    }

    memset(&sa, '\0', sizeof(sa));
    sa.sin_family = AF_INET;
    sa_sin_port = htons(port);
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);

    /* Set the flags and use nonblocking sockets. */
    flags = fcntl(s, F_GETFL, 0) flags |= O_NONBLOCK;
    fcntl(s, F_SETFL, flags);

    /* Now finally connect it. */
    connect(s, (struct sockaddr *)&sa, sizeof(sa));

    return connectionAdd("IRC Uplink", s, CF_CONNECTING);
}
#endif /* HAVE_GETADDRINFO */
