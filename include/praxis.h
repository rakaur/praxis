/*  praxis: services for TSora IRC networks.
 *  include/praxis.h: Contains general globals and such.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

/* *INDENT-OFF */

#ifndef INCLUDED_praxis_h
#define INCLUDED_praxis_h

#include "setup.h"

#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <grp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/socket.h>

#ifdef HARD_ASSERT
#define iassert assert
#else
#define iassert(expr)                                                         \
{                                                                             \
    do                                                                        \
    {                                                                         \
        if (!(expr))                                                          \
        {                                                                     \
            ilog(L_ERROR, "%s:%d: Assertion failed: (%s)",                    \
                 __FILE__, __LINE__, #expr);                                  \
        }                                                                     \
    } while (0);                                                              \
}
#endif

#ifndef HAVE_STRLCAT
size_t strlcat(char *dst, const char *src, size_t siz);
#endif
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#define BUFSIZE 512

#define NICKLEN    9            /* length of nicknames */
#define TOPICLEN   120          /* length of topics */
#define HOSTLEN    63           /* length of hostnames */
#define USERLEN    10           /* length of usernames */
#define REALLEN    50           /* length of gecos/real names */
#define CHANNELLEN 200          /* length of channel names */
#define PASSLEN    32           /* length of passwords */
#define KEYLEN     24           /* length of channel keys */

#ifdef LARGE_NETWORK
#define CHANNEL_HEAP_SIZE  1024
#define CHANUSER_HEAP_SIZE 1024
#define USER_HEAP_SIZE     1024
#define SERVER_HEAP_SIZE   32
#define UPLINK_HEAP_SIZE   4
#define CONN_HEAP_SIZE     32
#define EVENTQ_HEAP_SIZE   32
#define SENDQ_HEAP_SIZE    32
#define MODULE_HEAP_SIZE   128
#define DLINK_HEAP_SIZE    2048

#define SERVER_HASH_SIZE   32
#define SERVER_HASH_BITS   (32-5)
#define USER_HASH_SIZE     32768
#define USER_HASH_BITS     (32-15)
#define CHANNEL_HASH_SIZE  16384
#define CHANNEL_HASH_BITS  (32-14)
#define CMD_HASH_SIZE      387
#else
#define CHANNEL_HEAP_SIZE  64
#define CHANUSER_HEAP_SIZE 128
#define USER_HEAP_SIZE     128
#define SERVER_HEAP_SIZE   32
#define UPLINK_HEAP_SIZE   2
#define CONN_HEAP_SIZE     16
#define EVENTQ_HEAP_SIZE   16
#define SENDQ_HEAP_SIZE    16
#define MODULE_HEAP_SIZE   64
#define DLINK_HEAP_SIZE    1024

#define SERVER_HASH_SIZE   32
#define SERVER_HASH_BITS   (32-5)
#define USER_HASH_SIZE     4096
#define USER_HASH_BITS     (32-12)
#define CHANNEL_HASH_SIZE  1024
#define CHANNEL_HASH_BITS  (32-10)
#define CMD_HASH_SIZE      387
#endif

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long

extern const char *generation;
extern const char *creation;
extern const char *platform;
extern const char *version;
extern const char *infotext[];

typedef struct ModuleHeader ModuleHeader;

struct ModuleHeader
{
    const char *name;
    uchar (*moduleInit) ();
    void (*moduleFini) ();
};

struct cnt
{
    uint server;
    uint user;
    uint channel;

    uint connection;
    uint sendq;
    uint uplink;
    uint sra;

    uint node;
    uint timer;

    uint inb;
    uint outb;
} cnt;

struct me
{
    char name[HOSTLEN + 1];     /* our server name */
    char desc[REALLEN + 1];     /* free form description */
    char sid[4];                /* TS6 server id */

    char *admin_name;           /* our admin's name */
    char *admin_email;          /* our admin's email */
} me;

struct settings
{
    char vhost[HOSTLEN + 1];    /* ip or host to bind to */
    char *network_name;         /* the network's name */

    uint reconnect_time;        /* delay between reconnect attempts */
    uint ping_time;             /* delay between PINGs after no data */
    uint expire_time;           /* limit for inactive registrations */

    char *mta_path;             /* path to mail transfer agent */
    ushort auth_type;           /* email verification or not */

    ushort log_level;           /* logging level */
} settings;

#define AUTH_NONE  0
#define AUTH_EMAIL 1

struct globals
{
    time_t start;               /* TS we start running */
    time_t currtime;            /* current time */

    char config_file[256];      /* name/path of the config file */
    const char *last_timer;     /* name of the last Timer that ran */

    uint max_fd;                /* the highest fd we have open */
    uint uplink_failed;         /* failed uplink connection attempts */

    ushort run_flags;           /* our run flags */

    uchar connected;            /* if we're connected (0 or 1) */
    uchar bursting;             /* if we're bursting (0 or 1) */
} globals;

/* run flags */
#define RF_STARTING  0x01       /* starting up */
#define RF_REHASH    0x02       /* need a rehash */
#define RF_REHASHING 0x04       /* rehashing */
#define RF_RESTART   0x08       /* restart after shutdown */
#define RF_SHUTDOWN  0x10       /* shutdown */
#define RF_NOFORK    0x20       /* don't fork */

struct userserv
{
    uchar enabled;              /* if we're enabled (0 or 1) */

    char nick[NICKLEN + 1];     /* userserv's nickname */
    char user[USERLEN + 1];     /* userserv's username */
    char host[HOSTLEN + 1];     /* userserv's hostname */
    char real[REALLEN + 1];     /* userserv's "real name" (freeform) */

    ushort max_users;           /* max MyUser's per email address */

    uint default_flags;         /* MU_ upon register */
} userserv;

struct chanserv
{
    uchar enabled;              /* if we're enabled (0 or 1) */

    char nick[NICKLEN + 1];     /* chanserv's nickname */
    char user[USERLEN + 1];     /* chanserv's username */
    char host[HOSTLEN + 1];     /* chanserv's hostname */
    char real[REALLEN + 1];     /* chanserv's "real name" (freeform) */

    ushort max_chans;           /* max MyChannel's per MyUser */
    uchar join_chans;           /* join registered channels (0 or 1) */
    uchar part_chans;           /* part empty channels (0 or 1) */

    uint default_flags;         /* MC_ upon register */
} chanserv;

/* *INDENT-ON* */

#endif /* INCLUDED_praxis_h */
