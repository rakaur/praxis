/*  praxis: services for TSora IRC networks.
 *  src/uplink.c: Manages the uplink list.
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
#include "uplink.h"

static Heap *uplink_heap;

/* uplinkInit()
 *     Initialises the uplink_list.
 *
 * inputs     - none
 * outputs    - none
 */
void
uplinkInit(void)
{
    uplink_heap = ballocHeapCreate(sizeof(Uplink), UPLINK_HEAP_SIZE);
    memset(&uplink_list, '\0', sizeof(uplink_list));
    curr_uplink = NULL;

    if (uplink_heap == NULL)
    {
        ilog(L_INFO, "uplinkInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }
}

/* uplinkAdd()
 *     Adds an Uplink to the uplink_list.
 *
 * inputs     - name, host, password, port
 * outputs    - pointer to Uplink
 */
Uplink *
uplinkAdd(const char *name, const char *host, const char *pass, uint port)
{
    Uplink *uplink_p;

    iassert(name != NULL);
    iassert(host != NULL);
    iassert(pass != NULL);
    iassert(port != 0);

    ilog(L_DEBUG2, "uplinkAdd(): %s -> %s:%d", name, host, port);

    uplink_p = ballocHeapAlloc(uplink_heap);

    strlcpy(uplink_p->name, name, HOSTLEN);
    strlcpy(uplink_p->host, host, HOSTLEN);
    strlcpy(uplink_p->pass, pass, PASSLEN);
    uplink_p->port = port;

    /* We add Uplinks to the end of the list so that they're
     * "in order" with the configuration file entries.
     */
    dlinkAddTailAlloc(uplink_p, &uplink_list);

    cnt.uplink++;

    return uplink_p;
}

/* uplinkFind()
 *     Finds an Uplink in the uplink_list by "name".
 *
 * inputs     - name of Uplink
 * outputs    - pointer to Uplink or NULL on not found
 */
Uplink *
uplinkFind(const char *name)
{
    Uplink *uplink_p;
    DLinkNode *node_p;

    iassert(name != NULL);

    DLINK_FOREACH(node_p, uplink_list.head)
    {
        uplink_p = node_p->data;

        if (!strcasecmp(uplink_p->name, name))
            return uplink_p;
    }

    return NULL;
}

/* uplinkDelete()
 *     Deletes an Uplink from the uplink_list.
 *
 * inputs     - Uplink to delete
 * outputs    - 1 on success, 0 on failure
 */
uchar
uplinkDelete(Uplink *uplink_p)
{
    uchar ret = 0;

    iassert(uplink_p != NULL);

    ilog(L_DEBUG2, "uplinkDelete(): %s", uplink_p->name);

    ret = dlinkFindDestroy(uplink_p, &uplink_list);

    if (ret == 0)
    {
        ilog(L_DEBUG2, "uplinkDelete(): Cannot find Uplink: %s",
             uplink_p->name);

        return 0;
    }

    cnt.uplink--;

    ballocHeapFree(uplink_heap, uplink_p);

    return 1;
}

/* uplinkFlush()
 *     Deletes all entries in the uplink_list.
 *
 * inputs     - none
 * outputs    - 1 on success, 0 on failure
 */
uchar
uplinkFlush(void)
{
    Uplink *uplink_p;
    DLinkNode *node_p, *tnode_p;
    uchar ret = 0;

    ilog(L_DEBUG2, "uplinkFlush(): Flushing uplink_list");

    DLINK_FOREACH_SAFE(node_p, tnode_p, uplink_list.head)
    {
        uplink_p = node_p->data;

        ret = uplinkDelete(uplink_p);

        if (ret == 0)
        {
            ilog(L_DEBUG2, "uplinkFlush(): uplinkDelete() failed: %s",
                 uplink_p->name);

            return 0;
        }
    }

    return 1;
}

/* uplinkConnect()
 *     Chooses an Uplink in the uplink_list to connect to.
 *
 * inputs     - none
 * outputs    - pointer to Connection or NULL on failure
 */
Connection *
uplinkConnect(void)
{
    Uplink *uplink_p;
    Connection *connection_p;
    DLinkNode *node_p;

    iassert(globals.connected != 1);

    if (curr_uplink == NULL)
    {
        ilog(L_DEBUG2, "uplinkConnect(): Connecting to first entry");

        curr_uplink = uplink_list.head->data;
    }
    else
    {
        ilog(L_DEBUG2, "uplinkConnect(): Connecting to next entry");

        curr_uplink->connection_p = NULL;

        DLINK_FOREACH(node_p, uplink_list.head)
        {
            uplink_p = node_p->data;

            if (uplink_p == curr_uplink)
            {
                curr_uplink = NULL;
                continue;
            }

            if (curr_uplink == NULL)
            {
                curr_uplink = uplink_p;
                break;
            }
        }

        /* We've reached the last entry; go to the beginning. */
        if (curr_uplink == NULL)
        {
            ilog(L_DEBUG2, "uplinkConnect(): Reached end; cycling");
            curr_uplink = uplink_list.head->data;
        }
    }

    ilog(L_INFO, "uplinkConnect(): Connecting to %s (%s:%d)",
         curr_uplink->name, curr_uplink->host, curr_uplink->port);

    connection_p = netConnect(curr_uplink->host, curr_uplink->port,
                              (curr_uplink->vhost[0] != '\0') ?
                              curr_uplink->vhost : NULL);

    if (connection_p == NULL)
    {
        ilog(L_ERROR, "uplinkConnect(): Couldn't connect to %s (%s:%d)",
             curr_uplink->name, curr_uplink->host, curr_uplink->port);

        globals.uplink_failed++;

        if (dlinkLength(&uplink_list) <= globals.uplink_failed)
        {
            ilog(L_INFO, "uplinkConnect(): No uplinks work. Fix them.");
            exit(EXIT_FAILURE);
        }
    }

    connection_p->flags |= CF_UPLINK;
    curr_uplink->connection_p = connection_p;

    return connection_p;
}
