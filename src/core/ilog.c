/*  praxis: services for TSora IRC networks.
 *  src/ilog.c: Logging routines.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "ilog.h"

/* ilogInit()
 *     Initialises the logging system.
 *
 * inputs     - none
 * outputs    - none
 */
void
ilogInit(void)
{
    if (log_file != NULL)
        return;

    log_file = fopen("var/praxis.log", "a");

    if (log_file == NULL)
    {
        fprintf(stderr, "ilogInit(): Failed to open log file: var/praxis.log");
        exit(EXIT_FAILURE);
    }
}

/* ilogFini()
 *     Closes log file.
 *
 * inputs     - none
 * outputs    - none
 */
void
ilogFini(void)
{
    if (log_file == NULL)
        return;

    ilog(L_INFO, "ilogFini(): Closing log file.");

    fclose(log_file);
    log_file = NULL;
}

/* ilog()
 *     Writes something to the log file.
 *
 * inputs     - log level of type L_ bitmask, formatted log message.
 * outputs    - none
 */
void
ilog(ushort level, char *fmt, ...)
{
    va_list ap;
    time_t t = globals.currtime;
    struct tm tm;
    char buf[32];

    /* Check to see if our log file is even open. */
    if (log_file == NULL)
        return;

    /* L_DEBUG2 doesn't print L_DEBUG1 stuff. */
    if ((settings.log_level == L_DEBUG2) && (level == L_DEBUG1))
        return;

    /* Check to see if we should even bother. */
    if ((level > settings.log_level))
        return;

    va_start(ap, fmt);

    /* Format the current time. */
    tm = *localtime(&t);
    strftime(buf, (sizeof(buf) - 1), "[%d/%m %H:%M:%S] ", &tm);

    /* Write it to the log file. */
    fputs(buf, log_file);
    vfprintf(log_file, fmt, ap);
    fputc('\n', log_file);
    fflush(log_file);

    /* Print it to stderr if we're not forked or are starting up. */
    if ((globals.run_flags & (RF_NOFORK | RF_STARTING)))
    {
        vfprintf(stderr, fmt, ap);
        fputc('\n', stderr);
    }

    /* Clean up. */
    va_end(ap);
}

/* ilogRotate()
 *     Rotates log file.
 *
 * inputs     - none
 * outputs    - none
 */
void
ilogRotate(void)
{
    time_t t = (globals.currtime - 86400);
    struct tm tm;
    char buf[64];

    /* Format time for log file name. */
    tm = *localtime(&t);
    strftime(buf, 64, "praxis-%m-%d.log", &tm);
    ilog(L_INFO, "ilogRotate(): Rotating log file.");

    /* Close the log file, rename it, and reopen it. */
    ilogFini();
    rename("var/praxis.log", buf);
    ilogInit();
}
