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
 *  Channel management (channel.c)
 *  ---
 *  Handle channels
 *
 *  Created: 25.02.2011 16:08:16
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "channel.h"
#include "state.h"


int
channel_cmp(void *data, void *list_data)
{
    char *name = (char *)data;
    irc_channel *channel = (irc_channel *)list_data;

    return strcasecmp(name, channel->name);
}


int
user_cmp(void *data, void *list_data)
{
    char *name = (char *)data;
    irc_user *user = (irc_user *)list_data;

    return strcasecmp(name, user->nick);
}


void
channel_free(void *data)
{
    irc_channel *channel = (irc_channel *)data;
    int max = 64; /* TODO: calculate */
    int i;

    for (i = 0; i < max; ++i)
    {
        if (channel->flags[i].set)
        {
            if (channel->flags[i].type == FLAG_STRING)
                free(channel->flags[i].string);
            else if (channel->flags[i].type == FLAG_LIST)
                list_destroy(channel->flags[i].list, &free);
        }
    }

    list_destroy(channel->users, &free); /* No lists in irc_user -- free() */
    free(channel);

    return;
}


int
channel_add(luna_state *state, const char *channel_name)
{
    /* Maybe check whether this channel is already in - proper coding should
     * prevent this from happening anyways*/
    irc_channel *tmp = malloc(sizeof(*tmp));

    if (tmp)
    {
        memset(tmp, 0, sizeof(*tmp));

        strncpy(tmp->name, channel_name, sizeof(tmp->name) - 1);

        /* Try creating userlist, too */
        if (list_init(&(tmp->users)) != 0)
        {
            free(tmp);
            return 1;
        }

        list_push_back(state->channels, tmp);

        return 0;
    }

    return 1;
}


int
channel_remove(luna_state *state, const char *channel_name)
{
    /* Not really a reason to use actual types here */
    void *channel = NULL;
    void *key = (void *)channel_name;

    if ((channel = list_find(state->channels, key, &channel_cmp)) != NULL)
    {
        /* Channel was found */
        list_delete(state->channels, channel, &channel_free);

        return 0;
    }

    return 1;
}


int
channel_add_user(luna_state *state, const char *chan_name, const char *nick,
                 const char *user, const char *host)
{
    void *key = (void *)chan_name;
    void *channel = NULL;

    if ((channel = list_find(state->channels, key, &channel_cmp)) != NULL)
    {
        irc_channel *chan = (irc_channel *)channel;
        irc_user *tmp = malloc(sizeof(*tmp));

        if (tmp)
        {
            memset(tmp, 0, sizeof(*tmp));

            strncpy(tmp->nick, nick, sizeof(tmp->nick) - 1);
            strncpy(tmp->user, user, sizeof(tmp->user) - 1);
            strncpy(tmp->host, host, sizeof(tmp->host) - 1);

            list_push_back(chan->users, tmp);

            return 0;
        }
    }

    return 1;
}


int
channel_remove_user(luna_state *state, const char *chan_name, const char *nick)
{
    void *key_chan = (void *)chan_name;
    void *channel = NULL;

    if ((channel = list_find(state->channels, key_chan, &channel_cmp)) != NULL)
    {
        void *key_user = (void *)nick;
        void *user = NULL;

        irc_channel *chan = (irc_channel *)channel;

        if ((user = list_find(chan->users, key_user, &user_cmp)) != NULL)
        {
            /* User found */
            list_delete(chan->users, user, &free);

            return 0;
        }
    }

    return 1;
}


int
user_rename(luna_state *state, const char *oldnick, const char *newnick)
{
    list_node *cur = NULL;

    for (cur = state->channels->root; cur != NULL; cur = cur->next)
    {
        irc_channel *channel = (irc_channel *)(cur->data);
        void *key  = (void *)oldnick;
        void *user = NULL;

        if ((user = list_find(channel->users, key, &user_cmp)) != NULL)
        {
            irc_user *u = (irc_user *)user;

            memset(u->nick, 0, sizeof(u->nick));
            strncpy(u->nick, newnick, sizeof(u->nick) - 1);
        }
    }

    return 0;
}


int
channel_set_topic(luna_state *state, const char *channel, const char *topic)
{
    void *c = list_find(state->channels, (void *)channel, &channel_cmp);

    if (c)
    {
        irc_channel *chan = (irc_channel *)c;

        memset(chan->topic, 0, sizeof(chan->topic));
        strncpy(chan->topic, topic, sizeof(chan->topic) - 1);

        return 0;
    }

    return 1;
}


int
channel_set_creation_time(luna_state *state, const char *channel, time_t stamp)
{
    void *c = list_find(state->channels, (void *)channel, &channel_cmp);

    if (c)
    {
        irc_channel *chan = (irc_channel *)c;
        chan->created = stamp;

        return 0;
    }

    return 1;
}


int
channel_set_topic_meta(luna_state *state, const char *channel,
                       const char *setter, time_t time)
{
    void *c = list_find(state->channels, (void *)channel, &channel_cmp);

    if (c)
    {
        irc_channel *chan = (irc_channel *)c;

        memset(chan->topic_setter, 0, sizeof(chan->topic_setter));
        strncpy(chan->topic_setter, setter, sizeof(chan->topic_setter) - 1);

        chan->topic_set = time;

        return 0;
    }

    return 1;
}


irc_user *
channel_get_user(luna_state *state, const char *channel, const char *user)
{
    void *chan_data = list_find(state->channels, (void *)channel, &channel_cmp);

    if (chan_data)
    {
        irc_channel *chan = (irc_channel *)chan_data;

        void *user_data = list_find(chan->users, (void *)user, &user_cmp);

        if (user_data)
            return (irc_user *)user_data;
    }

    return NULL;
}


int
channel_op_user(luna_state *state, const char *channel, const char *user)
{
    irc_user *u = channel_get_user(state, channel, user);

    if (u)
    {
        u->op = 1;

        return 0;
    }

    return 1;
}


int
channel_deop_user(luna_state *state, const char *channel, const char *user)
{
    irc_user *u = channel_get_user(state, channel, user);

    if (u)
    {
        u->op = 0;

        return 0;
    }

    return 1;
}


int
channel_voice_user(luna_state *state, const char *channel, const char *user)
{
    irc_user *u = channel_get_user(state, channel, user);

    if (u)
    {
        u->voice = 1;

        return 0;
    }

    return 1;
}

int
channel_devoice_user(luna_state *state, const char *channel, const char *user)
{
    irc_user *u = channel_get_user(state, channel, user);

    if (u)
    {
        u->voice = 0;

        return 0;
    }

    return 1;
}
