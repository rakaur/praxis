/*  praxis: services for TSora IRC networks.
 *  include/user.h: Contains forward declarations for user.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_user_h
#define INCLUDED_user_h

typedef struct User User;

struct User
{
    char nick[NICKLEN + 1];     /* nickname */
    char user[USERLEN + 1];     /* username/ident */
    char host[HOSTLEN + 1];     /* hostname */
    char uid[10];               /* TS6 user id */

    Server *server_p;           /* pointer to Server */
//  MyUser *myuser_p;          /* pointer to MyUser (or NULL) */

    DLinkList channels;         /* list of joined channels */

    ushort fl_offenses;         /* number of times triggered */
    uint fl_msgs;               /* message number */
    time_t fl_lastmsg;          /* last message received time */
    time_t timestamp;           /* the timestamp */

    uint flags;                 /* UF_ bitmask */
    uint nick_hash;             /* hash id for quick reference */
    uint uid_hash;              /* hash id for quick reference */
};

#define UF_IRCOP 0x01
#define UF_ADMIN 0x02

DLinkList user_nick_hash[USER_HASH_SIZE];
DLinkList user_uid_hash[USER_HASH_SIZE];

#define userFindNickDelete(nick) userDelete(userFindNick(nick))
#define userFindUIDDelete(uid) userDelete(userFindUID(uid))

void userInit(void);
User *userAdd(const char *, const char *, const char *, Server *, const char *);
User *userFindNick(const char *);
User *userFindUID(const char *);
uchar userDelete(User *);
uchar userFlush(void);

#endif /* INCLUDED_user_h */
