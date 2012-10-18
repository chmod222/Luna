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
 *  Channel management (channel.h)
 *  ---
 *  Handle channels
 *
 *  Created: 25.02.2011 16:09:36
 *
 ******************************************************************************/
#ifndef CHANNEL_H
#define CHANNEL_H

#include <time.h>

#include "state.h"

#define FLAG(x) ((x) - 'A')

typedef enum flag_type
{
    FLAG_NONE, /* default for flags without args */
    FLAG_STRING,
    FLAG_LIST
} flag_type;


typedef struct flag
{
    int set;
    flag_type type;

    union
    {
        int bool;
        char *string;
        linked_list *list;
    };
} flag;


typedef struct irc_channel
{
    char name[64]; /* Some networks allow for more than 200 characters - adapt
                      if necessary! */
    char topic[512];
    char topic_setter[128];
    time_t topic_set;
    time_t created;

    linked_list *users;
    flag flags[64]; /* enough to cover flags [a-zA-Z] */
} irc_channel;


typedef struct irc_user
{
    char nick[32];
    char user[16];
    char host[128];

    char modes[16];
} irc_user;

int channel_cmp(const void *, const void *);
int user_cmp(const void *, const void *);

int channel_add(luna_state *, const char *);
int channel_remove(luna_state *, const char *);

int channel_add_user(luna_state *, const char *, const char *,
                     const char *, const char *);

int channel_remove_user(luna_state *, const char *, const char *);
int user_rename(luna_state *, const char *, const char *);

void channel_free(void *);

int channel_set_topic(luna_state *, const char *, const char *);
int channel_set_topic_meta(luna_state *, const char *, const char *, time_t);

int channel_set_creation_time(luna_state *, const char *, time_t);

irc_user *channel_get_user(luna_state *, const char *, const char *);

int channel_op_user(luna_state *, const char *, const char *);
int channel_deop_user(luna_state *, const char *, const char *);

int channel_voice_user(luna_state *, const char *, const char *);
int channel_devoice_user(luna_state *, const char *, const char *);

#endif
