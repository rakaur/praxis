/*  praxis: services for TSora IRC networks.
 *  src/user.c: Manages the user hash.
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
#include "user.h"

static Heap *user_heap;

/* userInit()
 *     Initialises the user_hash.
 *
 * inputs     - none
 * outputs    - none
 */
void
userInit(void)
{
    user_heap = ballocHeapCreate(sizeof(User), USER_HEAP_SIZE);
    memset(&user_nick_hash, '\0', sizeof(user_nick_hash));
    memset(&user_uid_hash, '\0', sizeof(user_uid_hash));

    if (user_heap == NULL)
    {
        ilog(L_INFO, "userInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }
}

/* userAdd()
 *     Adds a User to the user_hash.
 *
 * inputs     - nickname, username, hostname, server, UID or NULL
 * outputs    - pointer to User
 */
User *
userAdd(const char *nick, const char *user, const char *host, Server *server_p,
        const char *uid)
{
    User *user_p;

    iassert(nick != NULL);
    iassert(user != NULL);
    iassert(host != NULL);
    iassert(server_p != NULL);

    if (uid != NULL)
        ilog(L_DEBUG2, "userAdd() %s!%s@%s[%s] -> %s", nick, user, host, uid,
             server_p->name);
    else
        ilog(L_DEBUG2, "userAdd(): %s!%s@%s -> %s", nick, user, host,
             server_p->name);

    user_p = ballocHeapAlloc(user_heap);

    strlcpy(user_p->nick, nick, NICKLEN);
    strlcpy(user_p->user, user, USERLEN);
    strlcpy(user_p->host, host, HOSTLEN);

    if (uid != NULL)
        strlcpy(user_p->uid, uid, 10);

    user_p->server_p = server_p;

    if (uid != NULL)
        user_p->uid_hash = hashNick(uid);

    user_p->nick_hash = hashNick(nick);

    if (uid != NULL)
        dlinkAddAlloc(user_p, &user_uid_hash[user_p->uid_hash]);

    dlinkAddAlloc(user_p, &user_nick_hash[user_p->nick_hash]);
    dlinkAddAlloc(user_p, &server_p->user_list);

    cnt.user++;

    return user_p;
}

/* userFindNick()
 *     Finds a User in the user_nick_hash by "nick".
 *
 * inputs     - nickname of User
 * outputs    - pointer to User or NULL on not found
 */
User *
userFindNick(const char *nick)
{
    User *user_p;
    DLinkNode *node_p;
    uint hval;

    iassert(nick != NULL);

    hval = hashNick(nick);

    DLINK_FOREACH(node_p, user_nick_hash[hval].head)
    {
        user_p = node_p->data;

        if (!strcasecmp(user_p->nick, nick))
            return user_p;
    }

    return NULL;
}

/* userFindUID()
 *     Finds a User in the user_uid_hash by "uid".
 *
 * inputs     - UID of User
 * outputs    - pointer to User or NULL on not found
 */
User *
userFindUID(const char *uid)
{
    User *user_p;
    DLinkNode *node_p;
    uint hval;

    iassert(uid != NULL);

    hval = hashNick(uid);

    DLINK_FOREACH(node_p, user_uid_hash[hval].head)
    {
        user_p = node_p->data;

        if (!strcasecmp(user_p->uid, uid))
            return user_p;
    }

    return NULL;
}

/* userDelete()
 *     Deletes a User from the user_hash.
 *
 * inputs     - User to delete
 * outputs    - 1 on success, 0 on failure
 */
uchar
userDelete(User *user_p)
{
    uchar ret = 0;

    iassert(user_p != NULL);

    ilog(L_DEBUG2, "userDelete(): %s!%s@%s -> %s", user_p->nick,
         user_p->user, user_p->host, user_p->server_p->name);

    ret = dlinkFindDestroy(user_p, &user_p->server_p->user_list);

    if (ret == 0)
    {
        ilog(L_DEBUG2, "userDelete(): Cannot find User in Server: %s -> %s",
             user_p->nick, user_p->server_p->name);

        return 0;
    }

    ret = dlinkFindDestroy(user_p, &user_nick_hash[user_p->nick_hash]);

    if (ret == 0)
    {
        ilog(L_DEBUG2, "userDelete(): Cannot find User: %s", user_p->nick);

        return 0;
    }

    if (user_p->uid_hash != 0)
    {
        ret = dlinkFindDestroy(user_p, &user_uid_hash[user_p->uid_hash]);

        if (ret == 0)
        {
            ilog(L_DEBUG2, "userDelete(): Cannot find User: %s", user_p->uid);

            return 0;
        }
    }

    ballocHeapFree(user_heap, user_p);

    cnt.user--;

    return 1;
}

/* userFlush()
 *     Deletes all entries in the user_hash.
 *
 * inputs     - none
 * outputs    - 1 on success, 0 on failure
 */
uchar
userFlush(void)
{
    User *user_p;
    DLinkNode *node_p, *tnode_p;
    uint i;
    uchar ret = 0;

    ilog(L_DEBUG2, "userFlush(): Flushing user_hash");

    for (i = 0; i < USER_HASH_SIZE; i++)
    {
        DLINK_FOREACH_SAFE(node_p, tnode_p, user_nick_hash[i].head)
        {
            user_p = node_p->data;

            ret = userDelete(user_p);

            if (ret == 0)
            {
                ilog(L_DEBUG2, "userFlush(): userDelete() failed: %s",
                     user_p->nick);

                return 0;
            }
        }
    }

    return 1;
}
