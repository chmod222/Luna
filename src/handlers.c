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
#include "lua_api/lua_serializable.h"
#include "lua_api/lua_manager.h"
#include "lua_api/lua_util.h"
#include "lua_api/lua_source.h"
#include "lua_api/lua_script.h"
#include "lua_api/lua_channel.h"

void make_pair(luaX_channel*, luaX_chanuser*, const char *, const char *);

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

int handle_server_supports(luna_state *, irc_event *);

int handle_mode_change(luna_state *, const char *, const char *, char **, int);
int mode_set(luna_state *, const char *, char, const char *);
int mode_unset(luna_state *, const char *, char, const char *);


void
make_pair(luaX_channel *c, luaX_chanuser *u, const char *n, const char *ch)
{
    memset(c, 0, sizeof(*c));
    memset(u, 0, sizeof(*u));

    u->serialize = &luaX_push_chanuser;
    c->serialize = &luaX_push_channel;

    strncpy(c->name, ch, sizeof(c->name) - 1);
    strncpy(u->nick, n,  sizeof(u->nick) - 1);
    memcpy(&(u->channel), c, sizeof(u->channel));

    return;
}


int
handle_event(luna_state *env, irc_event *ev)
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
    char msgcopy[LINELEN];
    char *isitme = NULL;
    char *command = NULL;
    int priv;
    luna_server_support *chanprefixes =
        (luna_server_support *)list_find(env->server_support,
                                         "CHANTYPES", &support_by_key);

    if (chanprefixes)
        priv = strchr(chanprefixes->value, ev->param[0][0]) == NULL;
    else
        priv = ev->param[0][0] != '#';

    const char *sig = !priv ? "public_message" : "private_message";

    /* Make a copy of the message that we can modify without screwing
     * later operations */
    memset(msgcopy, 0, sizeof(msgcopy));

    strcpy(msgcopy, ev->msg);

    /* Check for a "<botnick>: <command> <...>" command */
    if  (((isitme = strtok(msgcopy, ":")) != NULL) &&
          (strcasecmp(isitme, env->userinfo.nick) == 0))
    {
        if ((command = strtok(NULL, " ")) != NULL)
            handle_command(env, ev, command, strtok(NULL, ""));
    }

    if (priv)
    {
        luaX_string s = luaX_make_string(ev->msg);
        luaX_source src = luaX_make_source(&(ev->from));

        signal_dispatch(env, sig, &src, &s, NULL);
    }
    else
    {
        luaX_channel ch;
        luaX_chanuser cu;
        luaX_string s = luaX_make_string(ev->msg);

        luaX_make_channel(&ch, ev->param[0]);
        luaX_make_chanuser(&cu, ev->from.nick, &ch);

        signal_dispatch(env, sig, &cu, &cu, &s, NULL);
    }

    return 0;
}


int
handle_command(luna_state *env, irc_event *ev, const char *cmd, char *rest)
{
    luna_server_support *chanprefixes =
        (luna_server_support *)list_find(env->server_support,
                                         "CHANTYPES", &support_by_key);
    int priv;

    if (chanprefixes)
        priv = strchr(chanprefixes->value, ev->param[0][0]) == NULL;
    else
        priv = ev->param[0][0] != '#';

    const char *sig = !priv ? "public_command" : "private_command";

    luna_user *user = user_match(env, &(ev->from));

    if ((user) && (strchr(user->flags, 'o')))
    {
        if (!strcasecmp(cmd, "load"))
        {
            char *script = strtok(rest, " ");
            handle_command_load(env, ev, script);
        }
        else if (!strcasecmp(cmd, "reload"))
        {
            char *script = strtok(rest, " ");
            handle_command_reload(env, ev, script);
        }
        else if (!strcasecmp(cmd, "unload"))
        {
            char *script = strtok(rest, " ");
            handle_command_unload(env, ev, script);
        }
        else if (!strcasecmp(cmd, "reloadusers"))
        {
            if (!users_reload(env, "users.txt"))
                net_sendfln(env, "PRIVMSG %s :%s: Reloaded %d users!",
                            ev->param[0], ev->from.nick, env->users->length);
            else
                net_sendfln(env, "PRIVMSG %s :%s: Failed to load users :(",
                            ev->param[0], ev->from.nick);
        }
    }

    if (priv)
    {
        luaX_string scmd = luaX_make_string(cmd);
        luaX_string srest = luaX_make_string(rest);
        luaX_source src = luaX_make_source(&(ev->from));

        signal_dispatch(env, sig, &src, &scmd, &srest, NULL);
    }
    else
    {
        luaX_chanuser cu;
        luaX_channel ch;

        luaX_string scmd = luaX_make_string(cmd);
        luaX_string srest = luaX_make_string(rest);

        make_pair(&ch, &cu, ev->from.nick, ev->param[0]);

        signal_dispatch(env, sig, &cu, &cu, &scmd, &srest, NULL);
    }

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
    char *mode = NULL;

    switch (ev->numeric)
    {
        case 5: /* ISUPPORT */
            handle_server_supports(env, ev);

            break;

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

            mode = ev->param[6];

            if (mode[1] == '@')
                channel_op_user(env, ev->param[1], ev->param[5]);
            else if (mode[1] == '+')
                channel_voice_user(env, ev->param[1], ev->param[5]);

            break;

        case 332: /* TOPIC */
            channel_set_topic(env, ev->param[1], ev->msg);

            break;

        case 333: /* TOPIC META */
            channel_set_topic_meta(env, ev->param[1],
                                   ev->param[2], atoi(ev->param[3]));

            break;

        case 324: /* REPL_MODE */
            /* <server> 324 <me> <channel> <flags> [param[,param[,...]]] */
            handle_mode_change(env, ev->param[1], ev->param[2], ev->param, 3);

            break;

        case 329: /* REPL_MODE2 */
            /* :aperture.esper.net 329 Luna^ #lulz2 1298798377 */
            channel_set_creation_time(env, ev->param[1], atoi(ev->param[2]));

            break;
    }

    /* TODO: dispatch template flag for string arrays */

    return 0;
}


int
handle_join(luna_state *env, irc_event *ev)
{
    const char *c = ev->msg ? ev->msg : ev->param[0];

    luaX_channel ch;
    luaX_chanuser cu;

    luaX_make_channel(&ch, c);
    luaX_make_chanuser(&cu, ev->from.nick, &ch);

    /* Is it me? */
    if (!strcasecmp(ev->from.nick, env->userinfo.nick))
    {
        /* Yes! Add channel to list */
        channel_add(env, c);

        /* Query userlist */
        net_sendfln(env, "WHO %s", c);
        net_sendfln(env, "MODE %s", c);
    }
    else
    {
        /* Nah, add user to channel */
        channel_add_user(env, c, ev->from.nick, ev->from.user, ev->from.host);
    }

    signal_dispatch(env, "channel_join", &cu, &ch, NULL);

    return 0;
}


int
handle_part(luna_state *env, irc_event *ev)
{
    luaX_channel ch;
    luaX_chanuser cu;

    luaX_make_channel(&ch, ev->param[0]);
    luaX_make_chanuser(&cu, ev->from.nick, &ch);

    if (ev->msg)
    {
        luaX_string reason = luaX_make_string(ev->msg);
        signal_dispatch(env, "channel_part", &cu, &ch, &reason, NULL);
    }
    else
    {
        signal_dispatch(env, "channel_part", &cu, &ch, NULL);
    }

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

    return 0;
}


int
handle_quit(luna_state *env, irc_event *ev)
{
    /* Remove user from all channels */
    list_node *cur;
    luaX_string reason = luaX_make_string(ev->msg);
    luaX_source src = luaX_make_source(&(ev->from));

    for (cur = env->channels->root; cur != NULL; cur = cur->next)
    {
        irc_channel *channel = (irc_channel *)(cur->data);
        channel_remove_user(env, channel->name, ev->from.nick);
    }

    signal_dispatch(env, "user_quit", &src, &reason, NULL);

    return 0;
}


int
handle_notice(luna_state *env, irc_event *ev)
{
    luaX_string target = luaX_make_string(ev->param[0]);
    luaX_string msg = luaX_make_string(ev->msg);
    luaX_source src = luaX_make_source(&(ev->from));

    signal_dispatch(env, "notice", &src, &target, &msg, NULL);

    return 0;
}


int
handle_nick(luna_state *env, irc_event *ev)
{
    const char *newnick = ev->msg ? ev->msg : ev->param[0];
    luaX_string nick = luaX_make_string(newnick);
    luaX_source src = luaX_make_source(&(ev->from));

    /* Is it me? */
    if (!strcasecmp(ev->from.nick, env->userinfo.nick))
    {
        /* Rename myself internally */
        memset(env->userinfo.nick, 0, sizeof(env->userinfo.nick));
        strncpy(env->userinfo.nick, newnick, sizeof(env->userinfo.nick) - 1);
    }

    /* Rename user in all channels */
    user_rename(env, ev->from.nick, newnick);

    signal_dispatch(env, "nick_change", &src, &nick, NULL);

    return 0;
}


int
handle_mode(luna_state *env, irc_event *ev)
{
    /* <sender> MODE <channel> <flags> [param[,param[,...]]] */

    /* If not me... */
    if (strcasecmp(ev->param[0], env->userinfo.nick))
        if (handle_mode_change(env, ev->param[0], ev->param[1], ev->param, 2))
            net_sendfln(env, "WHO %s", ev->param[0]);

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
    luaX_string channel = luaX_make_string(ev->msg);
    luaX_source src = luaX_make_source(&(ev->from));

    signal_dispatch(env, "invite", &src, &channel, NULL);

    return 0;
}


int
handle_topic(luna_state *env, irc_event *ev)
{
    char hoststring[128];

    luaX_channel ch;
    luaX_chanuser cu;
    luaX_string reason = luaX_make_string(ev->msg);

    luaX_make_channel(&ch, ev->param[0]);
    luaX_make_chanuser(&cu, ev->from.nick, &ch);

    memset(hoststring, 0, sizeof(hoststring));
    snprintf(hoststring, sizeof(hoststring), "%s!%s@%s",
             ev->from.nick, ev->from.user, ev->from.host);

    channel_set_topic(env, ev->param[0], ev->msg);
    channel_set_topic_meta(env, ev->param[0], hoststring, time(NULL));

    signal_dispatch(env, "topic_change", &cu, &ch, &reason, NULL);

    return 0;
}


int
handle_kick(luna_state *env, irc_event *ev)
{
    luaX_channel ch;
    luaX_chanuser kicker;
    luaX_chanuser kicked;
    luaX_string reason = luaX_make_string(ev->msg);

    luaX_make_channel(&ch, ev->param[0]);
    luaX_make_chanuser(&kicker, ev->from.nick, &ch);
    luaX_make_chanuser(&kicked, ev->param[1],  &ch);

    signal_dispatch(env, "user_kicked", &kicker, &ch, &kicked, &reason, NULL);

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

            luaX_script scrpt = luaX_make_script(script);

            net_sendfln(env, "PRIVMSG %s :%s: Loaded script '%s v%s'",
                        ev->param[0], ev->from.nick,
                        script->name, script->version);

            signal_dispatch(env, "script_load", &scrpt, NULL);
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
        net_sendfln(env, "PRIVMSG %s :%s: Script not loaded! Loading...",
                    ev->param[0], ev->from.nick);

        handle_command_load(env, ev, name);
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
            net_sendfln(env, "PRIVMSG %s :%s: Failed to reload script!",
                        ev->param[0], ev->from.nick);
        }
    }

    return 0;
}


int
handle_command_unload(luna_state *env, irc_event *ev, const char *name)
{
    void *loaded = list_find(env->scripts, (void *)name, &script_cmp);
    luna_script temp = *((luna_script*)loaded);

    if (!loaded)
        net_sendfln(env, "PRIVMSG %s :%s: Script not loaded!",
                    ev->param[0], ev->from.nick);
    else
        if (!script_unload(env, name))
        {
            luaX_script script = luaX_make_script(&temp);

            net_sendfln(env, "PRIVMSG %s :%s: Unloaded script!",
                        ev->param[0], ev->from.nick);

            signal_dispatch(env, "script_unload", &script, NULL);
        }
        else
        {
            net_sendfln(env, "PRIVMSG %s :%s: Failed to unload script!",
                        ev->param[0], ev->from.nick);
        }

    return 0;
}


int
handle_server_supports(luna_state *env, irc_event *ev)
{
    int i;

    for (i = 1; i < ev->param_count; ++i)
    {
        char *key = strtok(ev->param[i], "=");
        char *val = strtok(NULL, "");

        luna_server_support *ssp = malloc(sizeof(*ssp));
        if (ssp)
        {
            memset(ssp, 0, sizeof(*ssp));

            ssp->key = malloc(strlen(key) + 1);
            strcpy(ssp->key, key);

            if (val)
            {
                ssp->value = malloc(strlen(val) + 1);
                strcpy(ssp->value, val);
            }

            list_push_back(env->server_support, ssp);
        }

        /*if (strstr(ev->param[i], "CHANMODES="))
        {
            char *tok = NULL;
            char *part = NULL;
            int j;

            strtok(ev->param[i], "=");
            tok = strtok(NULL, "");

            part = strtok(tok, ",");

            for (j = 0; j < 4; ++j)
            {
                memset(env->chanmodes[j], 0, sizeof(env->chanmodes[j]));

                strncpy(env->chanmodes[j], part, sizeof(env->chanmodes[j]) - 1);
                part = strtok(NULL, ",");
            }
        }*/
    }

    return 0;
}


int
handle_mode_change(luna_state *state, const char *channel,
                   const char *flags, char **args, int argind)
{
    int action = 0; /* 0 = set, 1 = unset */
    int i = argind;
    int force_reload = 0;

    while (*flags)
    {
        switch (*flags)
        {
            case '+': action = 0; flags++; break;
            case '-': action = 1; flags++; break;
        }

        /* If...
         *   - Flag in chanmodes[0] (Address parameter, always) OR
         *   - Flag in chanmodes[1] (Setting, always) OR
         *   - Flag in chanmodes[2] and action is setting (Param when set) OR
         *   - Flag is either 'o' or 'v' which are not sent for chanmodes but
         *     behave like flags in chanmodes[1] */
        if  (strchr(state->chanmodes[0], *flags) ||
             strchr(state->chanmodes[1], *flags) ||
            (strchr(state->chanmodes[2], *flags) && !action) ||
            (*flags == 'o') ||
            (*flags == 'v'))
        {
            /* Set/Unset flag "*flags" with argument "args[i]" */
            if (!action)
                mode_set(state, channel, *flags, args[i++]);
            else
                mode_unset(state, channel, *flags, args[i++]);

            if (*flags == 'o')
                force_reload = 1;
        }
        else if ((strchr(state->chanmodes[3], *flags)) ||
                 (strchr(state->chanmodes[2], *flags) && action))
        {
            /* Set/Unset flag "*flags" for channel */
            if (!action)
                mode_set(state, channel, *flags, NULL);
            else
                mode_unset(state, channel, *flags, NULL);
        }

        flags++;
    }

    return force_reload;
}


int
mode_set(luna_state *state, const char *channel, char flag, const char *arg)
{
    if (arg)
    {
        switch (flag)
        {
            case 'o': return channel_op_user(state, channel, arg);
            case 'v': return channel_voice_user(state, channel, arg);
        }
    }

    return 0;
}


int
mode_unset(luna_state *state, const char *channel, char flag, const char *arg)
{
    if (arg)
    {
        switch (flag)
        {
            case 'o': return channel_deop_user(state, channel, arg);
            case 'v': return channel_devoice_user(state, channel, arg);
        }
    }

    return 0;
}
