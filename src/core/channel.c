/*  praxis: services for TSora IRC networks.
 *  src/channel.c: Manages the channel hash.
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
#include "channel.h"

static Heap *channel_heap;

/* channelInit()
 *     Initialises the channel_hash.
 *
 * inputs     - none
 * outputs    - none
 */
void
channelInit(void)
{
    channel_heap = ballocHeapCreate(sizeof(Channel), CHANNEL_HEAP_SIZE);
    memset(&channel_hash, '\0', sizeof(channel_hash));

    if (channel_heap == NULL)
    {
        ilog(L_INFO, "channelInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }
}

/* channelAdd()
 *     Adds a Channel to the channel_hash.
 *
 * inputs     - channel name
 * outputs    - pointer to Channel
 */
Channel *
channelAdd(const char *name)
{
    Channel *channel_p;

    iassert(name != NULL);

    ilog(L_DEBUG2, "channelAdd(): %s", name);

    channel_p = ballocHeapAlloc(channel_heap);

    strlcpy(channel_p->name, name, CHANNELLEN);

    channel_p->hash = hashChannel(name);
    dlinkAddAlloc(channel_p, &channel_hash[channel_p->hash]);

    cnt.channel++;

    return channel_p;
}

/* channelFind()
 *     Finds a Channel in the channel_hash by "name".
 *
 * inputs     - name of Channel
 * outputs    - pointer to Channel or NULL on not found
 */
Channel *
channelFind(const char *name)
{
    Channel *channel_p;
    DLinkNode *node_p;
    uint hval;

    iassert(name != NULL);

    hval = hashChannel(name);

    DLINK_FOREACH(node_p, channel_hash[hval].head)
    {
        channel_p = node_p->data;

        if (!strcasecmp(channel_p->name, name))
            return channel_p;
    }

    return NULL;
}

/* channelDelete()
 *     Deletes a Channel from the channel_hash.
 *
 * inputs     - Channel to delete
 * outputs    - 1 on success, 0 on failure
 */
uchar
channelDelete(Channel *channel_p)
{
    uchar ret = 0;

    iassert(channel_p != NULL);

    ilog(L_DEBUG2, "channelDelete(): %s", channel_p->name);

    ret = dlinkFindDestroy(channel_p, &channel_hash[channel_p->hash]);

    if (ret == 0)
    {
        ilog(L_DEBUG2, "channelDelete(): Cannot find Channel: %s",
             channel_p->name);

        return 0;
    }

    ballocHeapFree(channel_heap, channel_p);

    cnt.channel--;

    return 1;
}

/* channelFlush()
 *     Deletes all entries in the channel_hash.
 *
 * inputs     - none
 * outputs    - 1 on success, 0 on failure
 */
uchar
channelFlush(void)
{
    Channel *channel_p;
    DLinkNode *node_p, *tnode_p;
    uint i;
    uchar ret = 0;

    ilog(L_DEBUG2, "channelFlush(): Flushing channel_hash");

    for (i = 0; i < CHANNEL_HASH_SIZE; i++)
    {
        DLINK_FOREACH_SAFE(node_p, tnode_p, channel_hash[i].head)
        {
            channel_p = node_p->data;

            ret = channelDelete(channel_p);

            if (ret == 0)
            {
                ilog(L_DEBUG2, "channelFlush(): channelDelete() failed: %s",
                     channel_p->name);

                return 0;
            }
        }
    }

    return 1;
}
