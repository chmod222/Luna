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

#ifndef IRC_H
#define IRC_H

#include "net.h"

typedef enum irc_parse_status
{
    SOK,
    SINVALID,
    SNOMEM
} irc_parse_status;

typedef struct irc_message
{
    char *m_prefix;
    char *m_command;

    char **m_params;
    int m_paramcount;

    char *m_msg;
} irc_message;


irc_parse_status irc_parse_message(const char *, irc_message *);
void irc_free_message(irc_message *);
void irc_print_message(irc_message *);

int irc_user_cmp(const char *, const char *);

#endif
