/*  praxis: services for TSora IRC networks.
 *  include/events.h: Contains events defined for the event loop.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_events_h
#define INCLUDED_events_h

/* NOTE: Each event is assigned a number.  E_LAST must be one higher than any
 * other event defined or things will mess up in a very bad way: an array
 * bounds overflow.
 */

/* E_SHUTDOWN
 *
 * We're shutting down.  Typical things would be to remove the pid file and
 * call clean-up functions.
 */
#define E_SHUTDOWN		0

/* E_CONNECTED
 *
 * We've successfully connected to the active Uplink.
 */
#define E_CONNECTED		1

/* E_DISCONNECTED
 *
 * We've been disconnected from the active Uplink.
 */
#define E_DISCONNECTED		2

/* E_CONNDEAD
 *
 * A Connection has been marked as dead.
 */
#define E_CONNDEAD		3

/* E_CONNECTRETURN
 *
 * A connect()'d Connection is ready.
 */
#define E_CONNECTRETURN		4

/* E_LISTENRETURN
 *
 * A listen()'d Connection is ready.
 */
#define E_LISTENRETURN		5

/* E_DCCOUT
 *
 * A connect()'d DCC socket is ready.
 */
#define E_DCCOUT		6

/* E_DCCIN
 *
 * A listen()'d DCC socket is ready.
 */
#define E_DCCIN			7

/* E_SENDQREADY
 *
 * A Connection's SendQ is ready to be flushed.
 */
#define E_SENDQREADY		8

/* E_READYREADY
 *
 * A Connection has data to be read.
 */
#define E_READREADY		9

/* E_SELECTERROR
 *
 * select() has returned an error.
 */
#define E_SELECTERROR		10

/* E_PARSE
 *
 * A Connection has lines to parse.
 */
#define E_PARSE			11

/* E_NEWUSER
 *
 * A new user has been introduced.
 */
#define E_NEWUSER		12

/* E_USERQUIT
 *
 * A user has left the network.
 */
#define E_USERQUIT		13

/* E_LAST
 *
 * Last event; must be one higher than any other event.
 */
#define E_LAST			14

#endif /* INCLUDED_events_h */
