/*  praxis: services for TSora IRC networks.
 *  src/irc.c: IRC protocol interaction and message parsing.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "events.h"
#include "event.h"
#include "balloc.h"
#include "connection.h"
#include "hash.h"
#include "ilog.h"
#include "imem.h"
#include "send.h"
#include "server.h"
#include "uplink.h"
#include "irc.h"

/* ircTokenize()
 *     Splits apart a message into a parv.
 *
 * inputs     - message, parv
 * outputs    - parc
 */
static uint
ircTokenize(char *message, char *parv[])
{
    char *pos = NULL, *next = NULL;
    uint count = 0;

    iassert(message != NULL);

    /* First we find out if there's a colon in the message; we save that string
     * so we can set it to the last paramater in parv.
     * Also, make sure there's a space before it (IPv6).
     */
    pos = message;

    while (1)
    {
        pos = strchr(pos, ':');

        if (pos == NULL)
            break;

        pos--;

        if (*pos != ' ')
        {
            pos += 2;
            continue;
        }

        *pos = '\0';
        pos++;
        *pos = '\0';
        pos++;

        break;
    }

    /* Now we take the beginning of message and find all the spaces,
     * set them to \0 and use next to go through the string.
     */
    next = message;
    parv[0] = message;
    count = 1;

    while (*next)
    {
        if (count == 31)
        {
            ilog(L_DEBUG2, "ircTokenize(): Reached parameter limit");
            return count;
        }

        if (*next == ' ')
        {
            *next = '\0';
            next++;

            while (*next == ' ')
                next++;

            if (*next == '\0')
                break;

            parv[count] = next;
            count++;
        }
        else
            next++;
    }

    if (pos != NULL)
    {
        parv[count] = pos;
        count++;
    }

    return count;
}

/* ircParse()
 *     Parses incoming IRC messages.
 *
 * inputs     - Connection
 * outputs    - 1 on success or 0 on failure
 */
static uchar
ircParse(void *arg)
{
    Connection *connection_p = arg;
    CmdHashEntry *entry_p;
    DLinkNode *node_p, *tnode_p;
    char *line, *origin, *pos, *command, *message, *parv[32];
    static char core_line[BUFSIZE];
    uint parc, i;

    iassert(connection_p != NULL);

    line = origin = pos = command = message = NULL;
    parc = i = 0;

    /* Clear the parv. */
    for (i = 0; i < 32; i++)
        parv[i] = NULL;

    /* Go through each line in the queue. */
    DLINK_FOREACH(node_p, connection_p->recvq.head)
    {
        line = node_p->data;

        iassert(line != NULL);

        /* Sometimes we get just a blank line; catch those here
         * as they'll core us later on. */
        if ((*line == '\n') || (*line == '\000'))
            continue;

        /* Copy our original buffer for core files. */
        strlcpy(core_line, line, BUFSIZE);

        ilog(L_DEBUG1, "-> %s", core_line);

        /* Find the first space. */
        pos = strchr(line, ' ');

        if (pos != NULL)
        {
            *pos = '\0';
            pos++;

            /* If it starts with a ':' we have a prefix/origin.
             * Pull the origin off and have pos for the command.
             * Message will be the part afterwards.
             */
            if (*line == ':')
            {
                origin = line;
                origin++;

                message = strchr(pos, ' ');

                if (message != NULL)
                {
                    *message = '\0';
                    message++;
                    command = pos;
                }
                else
                {
                    command = pos;
                    message = NULL;
                }
            }
            else
            {
                message = pos;
                command = line;
            }
        }

        /* Now we make a parv out of what's left. */
        if (message != NULL)
        {
            if (*message == ':')
            {
                message++;
                parv[0] = message;
                parc = 1;
            }
            else
                parc = ircTokenize(message, parv);
        }
        else
            parc = 0;

        /* Now we have an origin, command, and a parv/parc... right? */
        if (command == NULL)
        {
            ilog(L_ERROR, "ircParse(): Command not found: %s", core_line);
            continue;
        }

        /* Take the command through the hash table to find our routine. */
        entry_p = irc_cmd_hash[hashCmd(command)];

        /* We don't know about this command. */
        if (entry_p == NULL)
            continue;

        entry_p->func(origin, parc, parv);

        /* Free the strdup() from io.c. */
        //free(node_p->data);
    }

    /* Destroy the recvq. */
    DLINK_FOREACH_SAFE(node_p, tnode_p, connection_p->recvq.head)
    {
        free(node_p->data);
        dlinkDestroy(node_p, &connection_p->recvq);
    }

    return 1;
}

/* *INDENT-OFF* */
static CmdHashEntry irc_commands[] =
{
    { "ERROR",  AC_NA, 0, m_error  },
    { "NICK",   AC_NA, 0, m_nick   },
    { "PASS",   AC_NA, 0, m_pass   },
    { "PING",   AC_NA, 0, m_ping   },
    { "QUIT",   AC_NA, 0, m_quit   },
    { "SERVER", AC_NA, 0, m_server },
    { "UID",    AC_NA, 0, m_uid    },
    { NULL,     0,     0, NULL     }
};
/* *INDENT-ON* */

/* ircInit()
 *     Initialises the irc_cmd_hash.
 *
 * inputs     - none
 * outputs    - none
 */
void
ircInit(void)
{
    uint i;

    memset(&irc_cmd_hash, '\0', sizeof(irc_cmd_hash));

    for (i = 0; irc_commands[i].func != NULL; i++)
        cmdhashAdd(&irc_commands[i], irc_cmd_hash);

    eventAddSpecial(E_PARSE, 1, ircParse);
}

/* cmdhashAdd()
 *     Adds a CmdHashEntry to a command hash.
 *
 * inputs     - CmdHashEntry, command hash
 * outputs    - 1 on success or 0 on failure
 */
uchar
cmdhashAdd(CmdHashEntry *entry_p, CmdHashEntry *cmdhash[])
{
    iassert(entry_p != NULL);
    iassert(cmdhash != NULL);

    ilog(L_DEBUG2, "cmdhashAdd(): Adding command %s", entry_p->command);

    entry_p->hash = hashCmd(entry_p->command);

    cmdhash[entry_p->hash] = entry_p;

    return 1;
}

/* cmdhashFind()
 *     Finds a CmdHashEntry in a command hash.
 *
 * inputs     - command name, command hash
 * outputs    - CmdHashEntry or NULL on failure
 */
CmdHashEntry *
cmdhashFind(const char *command, CmdHashEntry *cmdhash[])
{
    uint i;

    for (i = 0; i < CMD_HASH_SIZE; i++)
        if (!strcasecmp(command, irc_cmd_hash[i]->command))
            return irc_cmd_hash[i];

    return NULL;
}

void
cmdhashDelete(CmdHashEntry *entry_p, CmdHashEntry *cmdhash[])
{
    if (irc_cmd_hash[entry_p->hash] == entry_p)
        irc_cmd_hash[entry_p->hash] = NULL;
}
