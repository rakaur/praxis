/*  praxis: services for TSora IRC networks.
 *  src/praxis.c: Starts up and runs services.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "config.h"
#include "dlink.h"
#include "balloc.h"
#include "connection.h"
#include "events.h"
#include "event.h"
#include "hash.h"
#include "ilog.h"
#include "imem.h"
#include "io.h"
#include "module.h"
#include "send.h"
#include "sendq.h"
#include "server.h"
#include "timer.h"
#include "user.h"
#include "channel.h"
#include "irc.h"
#include "uplink.h"

#ifdef STATIC_MODULES
extern uchar moduleLoadStatic(void);
#endif

extern char **environ;

static void sigINT(int);
static void sigTERM(int);
static void sigHUP(int);

static void sigUSR1(int);
static void sigUSR2(int);

/* printHelp()
 *     Prints the help message.
 *
 * inputs     - none
 * outputs    - none
 */
static void
printHelp(void)
{
    printf("usage: praxis [-c config] [-d level] [-hnv]\n\n");

    printf("-c </path/to/file> - specify the configuration file\n");
    printf("-d <1|2|3>         - start in debugging mode\n");
    printf("-h                 - print the help message and exit\n");
    printf("-n                 - do not fork into the background\n");
    printf("-v                 - print the version information and exit\n");
}

/* main()
 *     Starts up and runs services.
 *
 * inputs     - argument count, argument vector
 * outputs    - return code to operating system
 */
int
main(int argc, char *argv[])
{
    Connection *connection_p;
    struct rlimit rlim;
    char buf[32];
    FILE *pid_file;
    pid_t pid;
    uchar have_conf = 0, ret;
    int r;

    /* Change to our local directory. */
    if (chdir(PREFIX) < 0)
    {
        perror(PREFIX);
        return 20;
    }

    /* Check to see if we're running as root. */
    if (geteuid() == 0)
    {
        fprintf(stderr, "praxis: do not run services as root!\n");
        return -1;
    }

    globals.currtime = time(NULL);
    globals.run_flags |= RF_STARTING;

    /* Make sure we leave core files. */
    if (getrlimit(RLIMIT_CORE, &rlim) == 0)
    {
        rlim.rlim_cur = rlim.rlim_max;
        setrlimit(RLIMIT_CORE, &rlim);
    }

    /* Set the default logging level. */
    settings.log_level = L_ERROR;

    /* Do command line options. */
    while ((r = getopt(argc, argv, "c:d:hnv")) != -1)
    {
        switch (r)
        {
            case 'c':
                strlcpy(globals.config_file, optarg, 256);
                have_conf = 1;
                break;

            case 'd':
                if (!strcmp(optarg, "1"))
                    settings.log_level = L_DEBUG1;
                else if (!strcmp(optarg, "2"))
                    settings.log_level = L_DEBUG2;
                else if (!strcmp(optarg, "3"))
                    settings.log_level = L_DEBUG3;
                else
                {
                    printf("usage: praxis [-c conf] [-d level] [-hnv]\n");
                    exit(EXIT_FAILURE);
                }

                break;

            case 'h':
                printHelp();
                exit(EXIT_SUCCESS);
                break;

            case 'n':
                globals.run_flags |= RF_NOFORK;
                break;

            case 'v':
                printf("praxis: version praxis-%s\n", version);
                exit(EXIT_SUCCESS);
                break;

            default:
                printf("usage: praxis [-c conf] [-d level] [-hnv]\n");
                exit(EXIT_SUCCESS);
                break;
        }
    }

    if (have_conf != 1)
        strlcpy(globals.config_file, "etc/praxis.conf", 256);

    /* Handle signals and such. */
    signal(SIGINT, sigINT);
    signal(SIGTERM, sigTERM);
    signal(SIGHUP, sigHUP);

    signal(SIGUSR1, sigUSR1);
    signal(SIGUSR2, sigUSR2);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGWINCH, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    printf("praxis: version praxis-%s\n", version);

    /* Check to see if we're already running. */
    pid_file = fopen("var/praxis.pid", "r");

    if (pid_file != NULL)
    {
        fgets(buf, 32, pid_file);

        if (buf != NULL)
        {
            pid = atoi(buf);

            if (kill(pid, 0) == 0)
            {
                fprintf(stderr, "praxis: daemon is already running\n");
                exit(EXIT_FAILURE);
            }
        }

        fclose(pid_file);
    }

    /* Initialise everything. */
    ilogInit();
    timerInit();
    ballocInit();
    dlinkInit();
    eventInit();
#ifndef STATIC_MODULES
    moduleInit();
#endif
    connectionInit();
    sendqInit();
    ioInit();
    ircInit();
    uplinkInit();
    serverInit();
    userInit();
    channelInit();

    /* Load our configuration file. */
    configInit();
    configParse();
    ret = configCheck();

    if (ret == 0)
    {
        fprintf(stderr, "praxis: errors in configuration\n");
        exit(EXIT_FAILURE);
    }

    /* Set up modules. */
#ifndef STATIC_MODULES
    moduleAddAll();

    ret = moduleLoadAll();
#else
    ret = moduleLoadStatic();
#endif

    if (ret == 0)
    {
        fprintf(stderr, "praxis: module failure (use -d2 for information)\n");
        exit(EXIT_FAILURE);
    }

    /* Fork into the background. */
    if (!(globals.run_flags & RF_NOFORK))
    {
        r = fork();

        if (r < 0)
        {
            fprintf(stderr, "praxis: cannot fork into the background\n");
            exit(EXIT_FAILURE);
        }

        else if (r != 0)
        {
            printf("praxis: pid %d\n", r);
            printf("praxis: running in background mode from %s\n", PREFIX);
            exit(EXIT_SUCCESS);
        }

        pid = setsid();

        if (pid == -1)
        {
            fprintf(stderr, "praxis: setsid() failed.\n");
            exit(EXIT_FAILURE);
        }

        /* Write praxis.pid. */
        pid_file = fopen("var/praxis.pid", "w");

        if (pid_file == NULL)
        {
            fprintf(stderr, "praxis: unable to writ epid file\n");
            exit(EXIT_FAILURE);
        }

        fprintf(pid_file, "%d\n", pid);
        fclose(pid_file);

        /* Close these useless things. */
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);
    }

    /* We're no longer starting. */
    globals.run_flags &= ~RF_STARTING;

    /* Connect to the IRC server. */
    while (1)
    {
        connection_p = uplinkConnect();

        if (connection_p != NULL)
            break;

        sleep(settings.reconnect_time);
    }

    globals.uplink_failed = 0;

    /* Start the in/out read/write loop. */
    io();

    /* We're shutting down. */

    if (globals.run_flags & RF_RESTART)
    {
        sendIRC(curr_uplink->connection_p,
                "ERROR :Closing Link: %s %s (Restarting.)",
                curr_uplink->host, curr_uplink->name);

        ilog(L_INFO, "main(): Restarting...");

        execve(argv[0], argv, environ);
    }

    sendIRC(curr_uplink->connection_p,
            "ERROR :Closing Link: %s %s (Terminating.)",
            curr_uplink->host, curr_uplink->name);

    ilog(L_INFO, "main(): Terminating; io() exited");

    remove("var/praxis.pid");

    return 0;
}

static void
sigINT(int signum)
{
    ilog(L_INFO, "sigINT(): Caught interrupt...");
    sendIRC(curr_uplink->connection_p,
            "ERROR :Closing Link: %s %s (Caught interrupt.)",
            curr_uplink->host, curr_uplink->name);

    globals.run_flags |= RF_SHUTDOWN;
}

static void
sigTERM(int signum)
{
    ilog(L_INFO, "sigTERM(): Terminating...");
    sendIRC(curr_uplink->connection_p,
            "ERROR :Closing Link: %s %s (Received SIGTERM.)",
            curr_uplink->host, curr_uplink->name);

    globals.run_flags |= RF_SHUTDOWN;
}

static void
sigHUP(int signum)
{
}

static void
sigUSR1(int signum)
{
    ilog(L_INFO, "sigUSR1(): Out of memory; exiting.");
    globals.run_flags |= RF_SHUTDOWN;
}

static void
sigUSR2(int signum)
{
    ilog(L_INFO, "sigUSR2(): Restarting...");
    globals.run_flags |= RF_RESTART;
}
