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
 *  Event handling (handlers.c)
 *  ---
 *  Takes care of core events
 *
 *  Created: 25.02.2011 14:37:59
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "irc.h"
#include "state.h"
#include "net.h"
#include "util.h"
#include "channel.h"
#include "lua_api.h"


int handle_ping(luna_state *,    irc_event *);
int handle_numeric(luna_state *, irc_event *);
int handle_privmsg(luna_state *, irc_event *);
int handle_join(luna_state *,    irc_event *);
int handle_part(luna_state *,    irc_event *);
int handle_quit(luna_state *,    irc_event *);
int handle_notice(luna_state *,  irc_event *);
int handle_nick(luna_state *,    irc_event *);
int handle_mode(luna_state *,    irc_event *);
int handle_invite(luna_state *,  irc_event *);
int handle_topic(luna_state *,   irc_event *);
int handle_kick(luna_state *,    irc_event *);
int handle_unknown(luna_state *, irc_event *);
int handle_command(luna_state *, irc_event *, const char *, char *);

int handle_command_load(luna_state *, irc_event *, const char *);
int handle_command_reload(luna_state *, irc_event *, const char *);
int handle_command_unload(luna_state *, irc_event *, const char *);

int handle_event(luna_state *env, irc_event *ev)
{
    /* Core event handlers */
    switch (ev->type)
    {
        case IRCEV_NUMERIC: return handle_numeric(env, ev);
        case IRCEV_PING:    return handle_ping(env, ev);
        case IRCEV_PRIVMSG: return handle_privmsg(env, ev);
        case IRCEV_JOIN:    return handle_join(env, ev);
        case IRCEV_PART:    return handle_part(env, ev);
        case IRCEV_QUIT:    return handle_quit(env, ev);
        case IRCEV_NOTICE:  return handle_notice(env, ev);
        case IRCEV_NICK:    return handle_nick(env, ev);
        case IRCEV_MODE:    return handle_mode(env, ev);
        case IRCEV_INVITE:  return handle_invite(env, ev);
        case IRCEV_TOPIC:   return handle_topic(env, ev);
        case IRCEV_KICK:    return handle_kick(env, ev);
        default:            return handle_unknown(env, ev);
    }

    return 0;
}


int
handle_privmsg(luna_state *env, irc_event *ev)
{
    char msgcopy[LINELEN + 1];
    char *isitme = NULL;
    char *command = NULL;
    const char *sig = ev->param[0][0] == '#' ? "public_message"
                                             : "private_message";

    /* Make a copy of the message that we can modify without screwing
     * later operations */
    strcpy(msgcopy, ev->msg);

    /* Check for a "<botnick>: <command> <...>" command */
    if  (((isitme = strtok(msgcopy, ":")) != NULL) && 
          (strcasecmp(isitme, env->userinfo.nick) == 0))
    {
        if ((command = strtok(NULL, " ")) != NULL)
            handle_command(env, ev, command, strtok(NULL, ""));
    }

    signal_dispatch(env, sig, "pss", &(ev->from), ev->param[0], ev->msg);

    return 0;
}


int
handle_command(luna_state *env, irc_event *ev, const char *cmd, char *rest)
{
    /* TODO: Handle script loading and unloading */

    const char *sig = ev->param[0][0] == '#' ? "public_command"
                                             : "private_command";

    if (!strcasecmp(cmd, "load"))
    {
        if (!user_match_level(env->users, &(ev->from), "admin"))
        {
            char *script = strtok(rest, " ");

            handle_command_load(env, ev, script);
        }

    }
    else if (!strcasecmp(cmd, "reload"))
    {
        if (!user_match_level(env->users, &(ev->from), "admin"))
        {
            char *script = strtok(rest, " ");

            handle_command_reload(env, ev, script);
        }
    }
    else if (!strcasecmp(cmd, "unload"))
    {
        if (!user_match_level(env->users, &(ev->from), "admin"))
        {
            char *script = strtok(rest, " ");

            handle_command_unload(env, ev, script);
        }
    }
    else if (!strcasecmp(cmd, "reloadusers"))
    {
        if (!user_match_level(env->users, &(ev->from), "admin"))
        {
            if (!users_reload(env->users, "users.txt"))
                net_sendfln(env, "PRIVMSG %s :%s: Reloaded %d users!",
                            ev->param[0], ev->from.nick, env->users->length);
            else
                net_sendfln(env, "PRIVMSG %s :%s: Failed to load users :(",
                            ev->param[0], ev->from.nick);
        }
    }

    signal_dispatch(env, sig, "psss", &(ev->from), ev->param[0], cmd, rest);

    return 0;
}


int
handle_ping(luna_state *env, irc_event *ev)
{
    const char *pingstr = (ev->param_count > 0)
        ? ev->param[0]
        : ev->msg;

    net_sendfln(env, "PONG :%s", pingstr);

    signal_dispatch(env, "ping", NULL);

    return 0;
}


int
handle_numeric(luna_state *env, irc_event *ev)
{
    switch (ev->numeric)
    {
        case 376:
            /* Dispatch connect event */
            signal_dispatch(env, "connect", NULL);

            break;

        case 352: /* REPL_WHO */
            /* param 0: me
             * param 1: channel
             * param 2: user
             * param 3: host
             * param 4: server
             * param 5: nick
             * param 6: modestr
             * msg: <hops> <realname> */
            channel_add_user(env, ev->param[1], ev->param[5], ev->param[2],
                             ev->param[3]);

            break;

        case 332: /* TOPIC */
            channel_set_topic(env, ev->param[1], ev->msg);

            break;

        case 333: /* TOPIC META */
            channel_set_topic_meta(env, ev->param[1],
                                   ev->param[2], atoi(ev->param[3]));

            break;
    }

    /* TODO: dispatch template flag for string arrays */

    return 0;
}


int
handle_join(luna_state *env, irc_event *ev)
{
    const char *c = ev->msg ? ev->msg : ev->param[0];

    /* Is it me? */
    if (!strcasecmp(ev->from.nick, env->userinfo.nick))
    {
        /* Yes! Add channel to list */
        channel_add(env, c);

        /* Query userlist */
        net_sendfln(env, "WHO %s", c);
    }
    else
    {
        /* Nah, add user to channel */
        channel_add_user(env, c, ev->from.nick, ev->from.user, ev->from.host);
    }

    signal_dispatch(env, "channel_join", "ps", &(ev->from), c);

    return 0;
}


int
handle_part(luna_state *env, irc_event *ev)
{
    /* Is it me? */
    if (!strcasecmp(ev->from.nick, env->userinfo.nick))
    {
        /* Yes! Remove channel from list */
        channel_remove(env, ev->param[0]);
    }
    else
    {
        /* Nah, remove user from channel */
        channel_remove_user(env, ev->param[0], ev->from.nick);
    }

    if (ev->msg)
        signal_dispatch(env, "channel_part", "pss", &(ev->from),
                        ev->param[0], ev->msg);
    else
        signal_dispatch(env, "channel_part", "ps", &(ev->from), ev->param[0]);

    return 0;
}


int
handle_quit(luna_state *env, irc_event *ev)
{
    /* Remove user from all channels */
    list_node *cur;

    for (cur = env->channels->root; cur != NULL; cur = cur->next)
    {
        irc_channel *channel = (irc_channel *)(cur->data);
        channel_remove_user(env, channel->name, ev->from.nick);
    }

    signal_dispatch(env, "user_quit", "ps", &(ev->from), ev->msg);

    return 0;
}


int
handle_notice(luna_state *env, irc_event *ev)
{
    signal_dispatch(env, "notice", "pss", &(ev->from), ev->param[0], ev->msg);

    return 0;
}


int
handle_nick(luna_state *env, irc_event *ev)
{
    const char *newnick = ev->msg ? ev->msg : ev->param[0];

    /* Is it me? */
    if (!strcasecmp(ev->from.nick, env->userinfo.nick))
    {
        /* Rename myself internally */
        memset(env->userinfo.nick, 0, sizeof(env->userinfo.nick));
        strncpy(env->userinfo.nick, newnick, sizeof(env->userinfo.nick) - 1);
    }

    /* Rename user in all channels */
    user_rename(env, ev->from.nick, newnick);

    signal_dispatch(env, "nick_change", "ps", &(ev->from), newnick);

    return 0;
}


int
handle_mode(luna_state *env, irc_event *ev)
{
    /* TODO: Later....
     * Format: 
     *    target = param[0]
     *    modestr = param[1]
     *    args = msg */

    return 0;
}


int
handle_invite(luna_state *env, irc_event *ev)
{
    signal_dispatch(env, "invite", "ps", &(ev->from), ev->param[1]);

    return 0;
}


int
handle_topic(luna_state *env, irc_event *ev)
{
    char hoststring[128];

    memset(hoststring, 0, sizeof(hoststring));
    snprintf(hoststring, sizeof(hoststring), "%s!%s@%s",
             ev->from.nick, ev->from.user, ev->from.host);

    channel_set_topic(env, ev->param[0], ev->msg);
    channel_set_topic_meta(env, ev->param[0], hoststring, time(NULL));

    signal_dispatch(env, "topic_change", "pss", &(ev->from),
                    ev->param[0], ev->msg);

    return 0;
}


int
handle_kick(luna_state *env, irc_event *ev)
{
    /* Remove user from all channels (ev->param[1]) */
    if (!strcasecmp(ev->param[1], env->userinfo.nick))
    {
        /* It's me! Geez! */
        channel_remove(env, ev->param[0]);
    }
    else
    {
        /* Remove user from channel! */
        channel_remove_user(env, ev->param[0], ev->param[1]);
    }

    signal_dispatch(env, "user_kicked", "psss", &(ev->from), ev->param[0],
                    ev->param[1], ev->msg);
    return 0;
}


int
handle_unknown(luna_state *env, irc_event *ev)
{
    return 0;
}


int
handle_command_load(luna_state *env, irc_event *ev, const char *name)
{
    void *already_loaded = list_find(env->scripts, (void *)name, &script_cmp);

    if (already_loaded)
    {
        net_sendfln(env, "PRIVMSG %s :%s: Script already loaded!",
                    ev->param[0], ev->from.nick);
    }
    else
    {
        if (!script_load(env, name))
        {
            void *loaded = list_find(env->scripts, (void *)name, &script_cmp);
            luna_script *script = (luna_script *)loaded;

            net_sendfln(env, "PRIVMSG %s :%s: Loaded script '%s v%s'",
                        ev->param[0], ev->from.nick,
                        script->name, script->version);
        }
        else
        {
            net_sendfln(env, "PRIVMSG %s :%s: Failed to load script!",
                        ev->param[0], ev->from.nick);
        }
    }

    return 0;
}


int
handle_command_reload(luna_state *env, irc_event *ev, const char *name)
{
    void *loaded = list_find(env->scripts, (void *)name, &script_cmp);

    if (!loaded)
    {
        net_sendfln(env, "PRIVMSG %s :%s: Script not loaded!",
                    ev->param[0], ev->from.nick);
    }
    else
    {
        if ((!script_unload(env, name)) && (!script_load(env, name)))
        {
            void *n = list_find(env->scripts, (void *)name, &script_cmp);
            luna_script *script = (luna_script *)n;

            net_sendfln(env, "PRIVMSG %s :%s: Reloaded script '%s v%s'",
                        ev->param[0], ev->from.nick,
                        script->name, script->version);
        }
        else
        {
            net_sendfln(env, "PRIVMSG %s :%s: Failed to unload script!",
                        ev->param[0], ev->from.nick);
        }
    }

    return 0;
}


int
handle_command_unload(luna_state *env, irc_event *ev, const char *name)
{
    void *loaded = list_find(env->scripts, (void *)name, &script_cmp);

    if (!loaded)
        net_sendfln(env, "PRIVMSG %s :%s: Script not loaded!",
                    ev->param[0], ev->from.nick);
    else
        if (!script_unload(env, name))
            net_sendfln(env, "PRIVMSG %s :%s: Unloaded script!",
                        ev->param[0], ev->from.nick);
        else
            net_sendfln(env, "PRIVMSG %s :%s: Failed to unload script!",
                        ev->param[0], ev->from.nick);

    return 0;
}
