/*  praxis: services for TSora IRC networks.
 *  include/irc.h: Contains forward declarations for irc.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_irc_h
#define INCLUDED_irc_h

#define cmdhashDestroy(entry, hash) do { cmdhashDelete(entry, hash); cmdhashFree(entry); } while (0);

typedef struct CmdHashEntry CmdHashEntry;

struct CmdHashEntry
{
    const char *command;
    uint level;
    uint hash;
    void (*func) (char *, uint, char **);
};

#define AC_NA    0x00
#define AC_NONE  0x01
#define AC_IRCOP 0x02
#define AC_SRA   0x04

CmdHashEntry *irc_cmd_hash[CMD_HASH_SIZE];

extern void m_error(char *, uint, char **);
extern void m_nick(char *, uint, char **);
extern void m_pass(char *, uint, char **);
extern void m_ping(char *, uint, char **);
extern void m_quit(char *, uint, char **);
extern void m_server(char *, uint, char **);
extern void m_uid(char *, uint, char **);

void ircInit(void);

uchar cmdhashAdd(CmdHashEntry *, CmdHashEntry **);
CmdHashEntry *cmdhashFind(const char *, CmdHashEntry **);
void cmdhashDelete(CmdHashEntry *, CmdHashEntry **);

#endif /* INCLUDED_irc_h */
