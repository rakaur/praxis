/*  praxis: services for TSora IRC networks.
 *  src/server.c: Manages the server hash.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "balloc.h"
#include "hash.h"
#include "ilog.h"
#include "server.h"

static Heap *server_heap;

/* serverInit()
 *     Initialises the server_hash.
 *
 * inputs     - none
 * outputs    - none
 */
void
serverInit(void)
{
    server_heap = ballocHeapCreate(sizeof(Server), SERVER_HEAP_SIZE);
    memset(&server_name_hash, '\0', sizeof(server_name_hash));
    memset(&server_sid_hash, '\0', sizeof(server_sid_hash));

    if (server_heap == NULL)
    {
        ilog(L_INFO, "serverInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }
}

/* serverAdd()
 *     Adds a Server to the server_hash.
 *
 * inputs     - name, SID or NULL
 * outputs    - pointer to Server
 */
Server *
serverAdd(const char *name, const char *sid)
{
    Server *server_p;

    iassert(name != NULL);

    if (sid != NULL)
        ilog(L_DEBUG2, "serverAdd(): %s[%s]", name, sid);
    else
        ilog(L_DEBUG2, "serverAdd(): %s", name);

    server_p = ballocHeapAlloc(server_heap);

    strlcpy(server_p->name, name, HOSTLEN);

    if (sid != NULL)
        strlcpy(server_p->sid, sid, 4);

    if (sid != NULL)
        server_p->sid_hash = hashServer(sid);

    server_p->name_hash = hashServer(name);

    if (sid != NULL)
        dlinkAddAlloc(server_p, &server_sid_hash[server_p->sid_hash]);

    dlinkAddAlloc(server_p, &server_name_hash[server_p->name_hash]);

    cnt.server++;

    return server_p;
}

/* serverFindName()
 *     Finds a Server in the server_name_hash by "name".
 *
 * inputs     - name of Server
 * outputs    - pointer to Server or NULL on not found
 */
Server *
serverFindName(const char *name)
{
    Server *server_p;
    DLinkNode *node_p;
    uint hval;

    iassert(name != NULL);

    hval = hashServer(name);

    DLINK_FOREACH(node_p, server_name_hash[hval].head)
    {
        server_p = node_p->data;

        if (!strcasecmp(server_p->name, name))
            return server_p;
    }

    return NULL;
}

/* serverFindSID()
 *     Finds a Server in the server_sid_hash by "sid".
 *
 * inputs     - SID of Server
 * outputs    - pointer to Server or NULL on not found
 */
Server *
serverFindSID(const char *sid)
{
    Server *server_p;
    DLinkNode *node_p;
    uint hval;

    iassert(sid != NULL);

    hval = hashServer(sid);

    DLINK_FOREACH(node_p, server_sid_hash[hval].head)
    {
        server_p = node_p->data;

        if (!strcasecmp(server_p->sid, sid))
            return server_p;
    }

    return NULL;
}

/* serverDelete()
 *     Deletes a Server from the server_hash.
 *
 * inputs     - Server to delete
 * outputs    - 1 on success, 0 on failure
 */
uchar
serverDelete(Server *server_p)
{
    uchar ret = 0;

    iassert(server_p != NULL);

    ilog(L_DEBUG2, "serverDelete(): %s", server_p->name);

    ret = dlinkFindDestroy(server_p, &server_name_hash[server_p->name_hash]);

    if (ret == 0)
    {
        ilog(L_DEBUG2, "serverDelete(): Cannot find Server: %s",
             server_p->name);

        return 0;
    }

    if (server_p->sid_hash != 0)
    {
        ret = dlinkFindDestroy(server_p, &server_sid_hash[server_p->sid_hash]);

        if (ret == 0)
        {
            ilog(L_DEBUG2, "serverDelete(): Cannot find Server: %s",
                 server_p->name);

            return 0;
        }
    }

    ballocHeapFree(server_heap, server_p);

    cnt.server--;

    return 1;
}

/* serverFlush()
 *     Deletes all entries in the server_hash.
 *
 * inputs     - none
 * outputs    - 1 on success, 0 on failure
 */
uchar
serverFlush(void)
{
    Server *server_p;
    DLinkNode *node_p, *tnode_p;
    uint i;
    uchar ret = 0;

    ilog(L_DEBUG2, "serverFlush(): Flushing server_hash");

    for (i = 0; i < SERVER_HASH_SIZE; i++)
    {
        DLINK_FOREACH_SAFE(node_p, tnode_p, server_name_hash[i].head)
        {
            server_p = node_p->data;

            ret = serverDelete(server_p);

            if (ret == 0)
            {
                ilog(L_DEBUG2, "serverFlush(): serverDelete() failed: %s",
                     server_p->name);

                return 0;
            }
        }
    }

    return 1;
}
