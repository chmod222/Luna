/*
 * This file is part of Luna
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/******************************************************************************
 *
 *  Basic IRC data structures (irc.h)
 *  ---
 *  IRC data structures
 *
 *  Created: 25.02.2011 15:12:26
 *
 ******************************************************************************/
#ifndef IRC_H
#define IRC_H

#include "net.h"

#define NICKLEN 32
#define IDENTLEN 16
#define HOSTLEN 128

#define MSGLEN LINELEN

#include "lua_api/lua_serializable.h"

typedef enum irc_event_type
{
    IRCEV_UNKNOWN,
    IRCEV_ERROR,
    IRCEV_NUMERIC,
    IRCEV_JOIN,
    IRCEV_PART,
    IRCEV_QUIT,
    IRCEV_PRIVMSG,
    IRCEV_NOTICE,
    IRCEV_NICK,
    IRCEV_MODE,
    IRCEV_PING,
    IRCEV_INVITE,
    IRCEV_TOPIC,
    IRCEV_KICK
} irc_event_type;

typedef struct irc_sender
{
    char nick[NICKLEN]; /* NULL if server */
    char user[IDENTLEN]; /* NULL if server */
    char host[HOSTLEN];
} irc_sender;

typedef struct irc_event
{
    irc_sender from;
    irc_event_type type;

    char **param;
    int param_count;
    char *msg;
    int numeric;
} irc_event;


int irc_parse_line(irc_event *, char *);
void irc_init_irc_event(irc_event *);
void irc_free_irc_event(irc_event *);
void irc_print_irc_event(irc_event *);

char *irc_event_type_to_string(irc_event_type);
enum irc_event_type irc_event_type_from_string(const char *);

#endif
