/*  praxis: services for TSora IRC networks.
 *  include/ilog.h: Contains forward declarations for ilog.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_ilog_h
#define INCLUDED_ilog_h

#define L_NONE   0              /* don't log anything */
#define L_INFO   1              /* log general info */
#define L_ERROR  2              /* L_INFO + errors */
#define L_DEBUG1 3              /* L_INFO + L_ERROR + io traffic */
#define L_DEBUG2 4              /* L_INFO + L_ERROR + function calls */
#define L_DEBUG3 5              /* L_INFO + L_ERROR + L_DEBUG1 + L_DEBUG2 */

FILE *log_file;

void ilogInit(void);
void ilogFini(void);
void ilog(ushort, char *, ...);
void ilogRotate(void);

#endif /* INCLUDED_ilog_h */
