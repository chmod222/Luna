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

/*******************************************************************************
 *
 *  Bot state (state.h)
 *  ---
 *  Define state to be passed around between the various functions
 *
 *  Created: 25.02.2011 12:29:33
 *
 ******************************************************************************/
#ifndef STATE_H
#define STATE_H

#include <time.h>

#include "logger.h"
#include "linked_list.h"


typedef struct luna_userinfo
{
    char nick[32];
    char user[16];
    char real[128];
} luna_userinfo;


typedef struct luna_serverinfo
{
    char host[128];
    unsigned short port;
} luna_serverinfo;


typedef struct luna_state
{
    luna_userinfo userinfo;
    luna_serverinfo serverinfo;

    luna_log *logger;

    int fd;
    int killswitch;

    time_t started;
    time_t connected;

    linked_list *channels;
    linked_list *scripts;
    linked_list *users;

    char *bind;

    char chanmodes[4][32];
} luna_state;


int state_init(luna_state *);
int state_destroy(luna_state *);

#endif
