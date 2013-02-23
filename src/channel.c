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

#include <stdlib.h>
#include <string.h>

#include "channel.h"
#include "state.h"
#include "util.h"
#include "mm.h"


/*
 * Linked list search comparators
 */
int channel_cmp(const void *data, const void *list_data)
{
    const char *name = (const char *)data;
    irc_channel *channel = (irc_channel *)list_data;

    return strcasecmp(name, channel->name);
}

int user_cmp(const void *data, const void *list_data)
{
    const char *name = (const char *)data;
    irc_user *user = (irc_user *)list_data;

    return irc_user_cmp(name, user->prefix);
}

/*
 * Linked list deletion deallocators
 */
void channel_free(void *data)
{
    irc_channel *channel = (irc_channel *)data;
    int max = 64;
    int i;

    for (i = 0; i < max; ++i)
    {
        if (channel->flags[i].set)
        {
            if (channel->flags[i].type == FLAG_STRING)
                mm_free(channel->flags[i].string);
            else if (channel->flags[i].type == FLAG_LIST)
                list_destroy(channel->flags[i].list, &mm_free);
        }
    }

    list_destroy(channel->users, &user_free);
    mm_free(channel);

    return;
}

void user_free(void *data)
{
    irc_user *user = (irc_user *)data;

    mm_free(user->prefix);
    mm_free(user);
}

/*
 * Channel management functions
 */
int channel_add(luna_state *state, const char *channel_name)
{
    irc_channel *tmp = (irc_channel *)list_find(state->channels,
                       (void *)channel_name, &channel_cmp);

    if (!tmp && (tmp = mm_malloc(sizeof(*tmp))))
    {
        strncpy(tmp->name, channel_name, sizeof(tmp->name) - 1);

        /* Try creating userlist, too */
        if (list_init(&(tmp->users)) != 0)
        {
            mm_free(tmp);
            return 1;
        }

        list_push_back(state->channels, tmp);

        return 0;
    }

    return 1;
}

int channel_remove(luna_state *state, const char *name)
{
    /* Not really a reason to use actual types here */
    void *channel = NULL;
    if ((channel = list_find(state->channels, name, &channel_cmp)) != NULL)
    {
        /* Channel was found */
        list_delete(state->channels, channel, &channel_free);

        return 0;
    }

    return 1;
}

irc_channel *channel_get(luna_state *state, const char *name)
{
    return list_find(state->channels, name, &channel_cmp);
}

int channel_set_topic(luna_state *state, const char *channel, const char *topic)
{
    irc_channel *chan = list_find(state->channels, channel, &channel_cmp);

    if (chan)
    {
        memset(chan->topic, 0, sizeof(chan->topic));
        strncpy(chan->topic, topic, sizeof(chan->topic) - 1);

        return 0;
    }

    return 1;
}

int channel_set_creation_time(luna_state *state, const char *channel, time_t t)
{
    irc_channel *chan = list_find(state->channels, channel, &channel_cmp);

    if (chan)
    {
        chan->created = t;

        return 0;
    }

    return 1;
}

int channel_set_topic_meta(luna_state *state, const char *channel,
                           const char *setter, time_t time)
{
    irc_channel *chan = list_find(state->channels, channel, &channel_cmp);

    if (chan)
    {
        memset(chan->topic_setter, 0, sizeof(chan->topic_setter));
        strncpy(chan->topic_setter, setter, sizeof(chan->topic_setter) - 1);

        chan->topic_set = time;

        return 0;
    }

    return 1;
}

/*
 * Channel user management functions
 */
int channel_add_user(luna_state *state, const char *chan_name, const char *pre)
{
    irc_channel *chan = NULL;

    if ((chan = list_find(state->channels, chan_name, &channel_cmp)) != NULL)
    {
        irc_user *tmp = (irc_user *)mm_malloc(sizeof(*tmp));

        if (tmp)
        {
            tmp->prefix = xstrdup(pre);
            list_push_back(chan->users, tmp);

            return 0;
        }
    }

    return 1;
}

int channel_remove_user(luna_state *state, const char *chan_name,
                        const char *nick)
{
    irc_channel *chan = NULL;

    if ((chan = list_find(state->channels, chan_name, &channel_cmp)) != NULL)
    {
        void *user = NULL;

        if ((user = list_find(chan->users, nick, &user_cmp)) != NULL)
        {
            /* User found */
            list_delete(chan->users, user, &user_free);

            return 0;
        }
    }

    return 1;
}

int channel_rename_user(luna_state *state, const char *oldprefix,
                        const char *newnick)
{
    list_node *cur = NULL;

    for (cur = state->channels->root; cur != NULL; cur = cur->next)
    {
        irc_channel *channel = (irc_channel *)(cur->data);
        irc_user *u = NULL;

        if ((u = list_find(channel->users, oldprefix, &user_cmp)) != NULL)
        {
            char *oldpost = strchr(u->prefix, '!');
            size_t len = strlen(oldpost) + strlen(newnick);;
            char *newpref = (char *)mm_malloc(len + 1);

            memset(newpref, 0, len + 1);
            strncat(newpref, newnick, len + 1);
            strncat(newpref, oldpost, len + 1);

            mm_free(u->prefix);
            u->prefix = newpref;
        }
    }

    return 0;
}

irc_user *channel_get_user(luna_state *state, const char *channel,
                           const char *user)
{
    irc_channel *chan = list_find(state->channels, channel, &channel_cmp);

    if (chan)
    {
        void *user_data = list_find(chan->users, (void *)user, &user_cmp);

        if (user_data)
            return (irc_user *)user_data;
    }

    return NULL;
}

