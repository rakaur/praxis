/*  praxis: services for TSora IRC networks.
 *  include/channel.h: Contains forward declarations for user.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_channel_h
#define INCLUDED_channel_h

typedef struct Channel Channel;

struct Channel
{
    char name[CHANNELLEN + 1];  /* channel name */
    char key[KEYLEN + 1];       /* cmode +k key (or NULL) */

//  MyChannel *mychannel_p;       /* pointer to MyChannel (or NULL) */

    DLinkList member_list;      /* list of joined users */

    uint limit;                 /* cmode +l limit (or 0) */
    uint modes;                 /* CMODE_ bitmask */
    time_t timestamp;           /* creation date */

    uint hash;                  /* hash id for quick reference */
};

DLinkList channel_hash[CHANNEL_HASH_SIZE];

#define channelFindDelete(name) channelDelete(channelFind(name))

void channelInit(void);
Channel *channelAdd(const char *);
Channel *channelFind(const char *);
uchar channelDelete(Channel *);
uchar channelFlush(void);

#endif /* INCLUDED_channel_h */
