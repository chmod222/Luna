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
#include <ctype.h>

#include "irc.h"
#include "state.h"
#include "net.h"
#include "util.h"
#include "channel.h"
#include "mm.h"

#include "lua_api/lua_manager.h"
#include "lua_api/lua_util.h"


char *_strdup(const char *);

int handle_ping(luna_state *,    irc_message *);
int handle_numeric(luna_state *, irc_message *);
int handle_privmsg(luna_state *, irc_message *);
int handle_join(luna_state *,    irc_message *);
int handle_part(luna_state *,    irc_message *);
int handle_quit(luna_state *,    irc_message *);
int handle_notice(luna_state *,  irc_message *);
int handle_nick(luna_state *,    irc_message *);
int handle_mode(luna_state *,    irc_message *);
int handle_invite(luna_state *,  irc_message *);
int handle_topic(luna_state *,   irc_message *);
int handle_kick(luna_state *,    irc_message *);
int handle_unknown(luna_state *, irc_message *);
int handle_command(luna_state *, irc_message *, const char *, char *);
int handle_ctcp(luna_state *,    irc_message *, const char *, char *);
int handle_action(luna_state *,  irc_message *, const char *);
int handle_server_supports(luna_state *, irc_message *);
int handle_mode_change(luna_state *, const char *, const char *, char **, int);

int mode_set(luna_state *, const char *, char, const char *);
int mode_unset(luna_state *, const char *, char, const char *);


int handle_event(luna_state *env, irc_message *ev)
{
    signal_dispatch(env, "raw", &luaX_push_raw, ev, NULL);

    /* Core event handlers */
    if (isdigit(*(ev->m_command)))
        return handle_numeric(env, ev);
    else if (!strcmp(ev->m_command, "PING"))
        return handle_ping(env, ev);
    else if (!strcmp(ev->m_command, "PRIVMSG"))
        return handle_privmsg(env, ev);
    else if (!strcmp(ev->m_command, "JOIN"))
        return handle_join(env, ev);
    else if (!strcmp(ev->m_command, "PART"))
        return handle_part(env, ev);
    else if (!strcmp(ev->m_command, "QUIT"))
        return handle_quit(env, ev);
    else if (!strcmp(ev->m_command, "NOTICE"))
        return handle_notice(env, ev);
    else if (!strcmp(ev->m_command, "NICK"))
        return handle_nick(env, ev);
    else if (!strcmp(ev->m_command, "MODE"))
        return handle_mode(env, ev);
    else if (!strcmp(ev->m_command, "INVITE"))
        return handle_invite(env, ev);
    else if (!strcmp(ev->m_command, "TOPIC"))
        return handle_topic(env, ev);
    else if (!strcmp(ev->m_command, "KICK"))
        return handle_kick(env, ev);
    else
        return handle_unknown(env, ev);

    return 0;
}

int handle_privmsg(luna_state *env, irc_message *ev)
{
    char *msgcopy;
    char *isitme = NULL;
    char *command = NULL;
    int priv;

    /* Needs at least the target */
    if (ev->m_paramcount < 1)
        return 1;

    priv = strchr(env->chantypes, ev->m_params[0][0]) == NULL;

    /* Make a copy of the message that we can modify without screwing
     * later operations */
    if ((msgcopy = xstrdup(ev->m_msg)) == NULL)
        return 1;

    /* Check for CTCP */
    if ((ev->m_msg[0] == 0x01) && (ev->m_msg[strlen(ev->m_msg) - 1] == 0x01))
    {
        char *ctcp = NULL;
        char *args = NULL;
        char *j = NULL;

        ev->m_msg[strlen(ev->m_msg) - 1] = '\0';
        j = ev->m_msg + strlen(ev->m_msg) - 1;

        while ((j >= ev->m_msg) && isspace(*j))
            j--;

        *(j + 1) = 0;

        ctcp = strtok(ev->m_msg + 1, " ");
        args = strtok(NULL, "");

        return handle_ctcp(env, ev, ctcp, args);
    }

    /* Check for a "<botnick>: <command> <...>" command */
    // TODO: Make trigger configurable
    if  (((isitme = strtok(msgcopy, ":")) != NULL) &&
            (strcasecmp(isitme, env->userinfo.nick) == 0))
    {
        if ((command = strtok(NULL, " ")) != NULL)
            return handle_command(env, ev, command, strtok(NULL, ""));
    }

    if (priv)
        signal_dispatch(env, "private_message", &luaX_push_privmsg, ev, NULL);
    else
        signal_dispatch(env, "public_message", &luaX_push_privmsg, ev, NULL);

    return 0;
}

int handle_ctcp(luna_state *env, irc_message *ev, const char *ctcp, char *msg)
{
    int priv = strchr(env->chantypes, ev->m_params[0][0]) == NULL;

    /* Special case for /ME commands */
    if (!strcmp(ctcp, "ACTION"))
        return handle_action(env, ev, msg);

    if (priv)
    {
        if (!strcmp(ev->m_command, "PRIVMSG"))
            signal_dispatch(env, "private_ctcp", &luaX_push_ctcp,
                    ev, ctcp, msg, NULL);

        else if (!strcmp(ev->m_command, "NOTICE"))
            signal_dispatch(env, "private_ctcp_response", &luaX_push_ctcp_rsp,
                    ev, ctcp, msg, NULL);
    }
    else
    {
        if (!strcmp(ev->m_command, "PRIVMSG"))
            signal_dispatch(env, "public_ctcp", &luaX_push_ctcp,
                    ev, ctcp, msg, NULL);

        else if (!strcmp(ev->m_command, "NOTICE"))
            signal_dispatch(env, "public_ctcp_response", &luaX_push_ctcp_rsp,
                    ev, ctcp, msg, NULL);
    }

    return 0;
}

int handle_action(luna_state *env, irc_message *ev, const char *message)
{
    int priv = strchr(env->chantypes, ev->m_params[0][0]) == NULL;

    if (priv)
        signal_dispatch(env, "private_action", &luaX_push_action,
                ev, message, NULL);
    else
        signal_dispatch(env, "public_action", &luaX_push_action,
                ev, message, NULL);

    return 0;
}

int handle_command(luna_state *env, irc_message *ev, const char *cmd, char *rest)
{
    int priv = strchr(env->chantypes, ev->m_params[0][0]) == NULL;

    if (priv)
        signal_dispatch(env, "private_command", &luaX_push_command,
                ev, cmd, rest, NULL);
    else
        signal_dispatch(env, "public_command", &luaX_push_command,
                ev, cmd, rest, NULL);

    return 0;
}

int handle_ping(luna_state *env, irc_message *ev)
{
    const char *pingstr;

    if ((ev->m_msg == NULL) && (ev->m_paramcount < 1))
        return 1;

    pingstr = (ev->m_paramcount > 0) ? ev->m_params[0] : ev->m_msg;
    net_sendfln(env, "PONG :%s", pingstr);

    signal_dispatch(env, "ping", NULL);

    return 0;
}

void print_user(void *_env, void *_user)
{
    luna_state *env = _env;
    irc_user *user = _user;

    logger_log(env->logger, LOGLEV_DEBUG, "-> '%s' = %s", user->prefix, user->modes);
}

int handle_numeric(luna_state *env, irc_message *ev)
{
    int i = 0;
    int numeric = atoi(ev->m_command);

    irc_user *target = NULL;

    switch (numeric)
    {
    case 5: /* ISUPPORT */
        handle_server_supports(env, ev);

        break;

    case 315: /* WHO End */

        /* param 0: me
         * param 1: channel
         */
        if (ev->m_paramcount < 2)
            return 1;

        target = channel_get_user(env, ev->m_params[1], ev->m_params[0]);
        irc_channel *chan = channel_get(env, ev->m_params[1]);

        logger_log(env->logger, LOGLEV_DEBUG, "Joined channel '%s'",
                chan->name);
        list_map(chan->users, &print_user, env);
        logger_log(env->logger, LOGLEV_DEBUG, "Topic: '%s' (Set %d by %s)",
                chan->topic, chan->topic_set, chan->topic_setter);

        net_sendfln(env, "NICK Lunaa");

        if (target)
            signal_dispatch(env, "channel_join_sync", &luaX_push_join_sync,
                            ev, NULL);

        break;

    case 376:
        /* Dispatch connect event */
        signal_dispatch(env, "connect", NULL);
        net_sendfln(env, "JOIN #luna");

        break;

    case 352: /* RPL_WHO */

        /* param : me
         * param : channel
         * param : user
         * param : host
         * param : server
         * param : nick
         * param : modestr
         * msg: <ops> <realname> */
        if (ev->m_paramcount < 7)
            return 1;

        char prefix[128] = {0};
        snprintf(prefix, sizeof(prefix), "%s!%s@%s",
                ev->m_params[5],
                ev->m_params[2],
                ev->m_params[3]);

        channel_add_user(env, ev->m_params[1], prefix);
        target = channel_get_user(env, ev->m_params[1], ev->m_params[5]);

        if (target)
        {
            int max = sizeof(env->userprefix) / sizeof(env->userprefix[0]);
            char *mode = ev->m_params[6];

            while (*mode++)
            {
                for (i = 0; ((i < max) && (env->userprefix[i].prefix != 0));
                        ++i)
                {
                    if (env->userprefix[i].prefix == *mode)
                    {
                        int len = strlen(target->modes);
                        target->modes[len] = env->userprefix[i].mode;
                        target->modes[len + 1] = 0;
                    }
                }
            }
        }

        break;

    case 332: /* TOPIC */

        if (ev->m_paramcount < 2)
            return 1;

        channel_set_topic(env, ev->m_params[1], ev->m_msg);

        break;

    case 333: /* TOPIC META */

        if (ev->m_paramcount < 4)
            return 1;

        channel_set_topic_meta(env,
                ev->m_params[1],
                ev->m_params[2],
                atoi(ev->m_params[3]));

        break;

    case 324: /* REPL_MODE */

        /* <server> 324 <me> <channel> <flags> [param[,param[,...]]] */
        if (ev->m_paramcount < 3)
            return 1;

        handle_mode_change(env,
                ev->m_params[1],
                ev->m_params[2],
                ev->m_params, 3);

        break;

    case 329: /* REPL_MODE2 */

        /* :aperture.esper.net 329 Luna^ #lulz2 1298798377 */
        if (ev->m_paramcount < 3)
            return 1;

        channel_set_creation_time(env, ev->m_params[1], atoi(ev->m_params[2]));

        break;
    }

    return 0;
}

int handle_join(luna_state *env, irc_message *ev)
{
    const char *c;

    /*
     * Some IRC servers are weird like that and put the joined channel
     * into the trailing message.
     */
    if ((ev->m_msg == NULL) && ev->m_paramcount < 1)
        return 1;

    c = ev->m_msg ? ev->m_msg : ev->m_params[0];

    /* Is it me? */
    if (!irc_user_cmp(ev->m_prefix, env->userinfo.nick))
    {
        /* Yes! Add channel to list */
        channel_add(env, c);

        /* Query userlist and modes */
        net_sendfln(env, "MODE %s", c);
        net_sendfln(env, "WHO %s", c);
    }
    else
    {
        /* Nah, add user to channel */
        channel_add_user(env, c, ev->m_prefix);

        signal_dispatch(env, "channel_join", &luaX_push_join, ev, NULL);
    }

    return 0;
}

int handle_part(luna_state *env, irc_message *ev)
{
    if (ev->m_paramcount < 1)
        return 1;

    signal_dispatch(env, "channel_part", &luaX_push_part, ev, NULL);

    /* Is it me? */
    if (!irc_user_cmp(ev->m_prefix, env->userinfo.nick))
        /* Yes! Remove channel from list */
        channel_remove(env, ev->m_params[0]);
    else
        /* Nah, remove user from channel */
        channel_remove_user(env, ev->m_params[0], ev->m_prefix);

    return 0;
}

int handle_quit(luna_state *env, irc_message *ev)
{
    /* Remove user from all channels */
    list_node *cur;

    for (cur = env->channels->root; cur != NULL; cur = cur->next)
    {
        irc_channel *channel = (irc_channel *)(cur->data);
        channel_remove_user(env, channel->name, ev->m_prefix);
    }

    signal_dispatch(env, "user_quit", &luaX_push_quit, ev, NULL);

    return 0;
}

int handle_notice(luna_state *env, irc_message *ev)
{
    int priv;

    if (ev->m_paramcount < 1)
        return 1;

    if (env->chantypes)
        priv = strchr(env->chantypes, ev->m_params[0][0]) == NULL;
    else
        priv = 1;

    /* CTCP response? */
    if ((ev->m_msg[0] == 0x01) && (ev->m_msg[strlen(ev->m_msg) - 1] == 0x01))
    {
        char *ctcp;
        char *args;
        char *j;

        ev->m_msg[strlen(ev->m_msg) - 1] = '\0';

        j = ev->m_msg + strlen(ev->m_msg) - 1;

        while ((j >= ev->m_msg) && isspace(*j))
            j--;

        *(j + 1) = 0;

        ctcp = strtok(ev->m_msg + 1, " ");
        args = strtok(NULL, "");

        return handle_ctcp(env, ev, ctcp, args);
    }
    else
    {
        if (priv)
            signal_dispatch(env, "private_notice", &luaX_push_notice, ev, NULL);
        else
            signal_dispatch(env, "public_notice", &luaX_push_notice, ev, NULL);
    }

    return 0;
}

int handle_nick(luna_state *env, irc_message *ev)
{
    const char *newnick;

    /*
     * Again, weird IRC servers.
     */
    if ((ev->m_msg == NULL) && (ev->m_paramcount < 1))
        return 1;

    newnick = ev->m_msg ? ev->m_msg : ev->m_params[0];

    /* Is it me? */
    if (!irc_user_cmp(ev->m_prefix, env->userinfo.nick))
    {
        /* Rename myself internally */
        memset(env->userinfo.nick, 0, sizeof(env->userinfo.nick));
        strncpy(env->userinfo.nick, newnick, sizeof(env->userinfo.nick) - 1);
    }

    /* Rename user in all channels */
    channel_rename_user(env, ev->m_prefix, newnick);
    signal_dispatch(env, "nick_change", &luaX_push_nick, ev, newnick, NULL);

    return 0;
}

int handle_mode(luna_state *env, irc_message *ev)
{
    /* <sender> MODE <channel> <flags> [param[,param[,...]]] */

    if (ev->m_paramcount < 2)
        return 1;

    /* If not me... */
    if (strcasecmp(ev->m_params[0], env->userinfo.nick))
        handle_mode_change(env,
                ev->m_params[0],
                ev->m_params[1],
                ev->m_params, 2);

    return 0;
}

int handle_invite(luna_state *env, irc_message *ev)
{
    signal_dispatch(env, "invite", &luaX_push_invite, ev, NULL);

    return 0;
}

int handle_topic(luna_state *env, irc_message *ev)
{
    if (ev->m_paramcount < 1)
        return 1;

    channel_set_topic(env, ev->m_params[0], ev->m_msg);
    channel_set_topic_meta(env, ev->m_params[0], ev->m_prefix, time(NULL));

    signal_dispatch(env, "topic_change", &luaX_push_topic, ev, NULL);

    return 0;
}

int handle_kick(luna_state *env, irc_message *ev)
{
    if (ev->m_paramcount < 2)
        return 1;

    signal_dispatch(env, "user_kicked", &luaX_push_kick, ev, NULL);

    /* Remove user from all channels (ev->m_params[1]) */
    if (!irc_user_cmp(ev->m_params[1], env->userinfo.nick))
    {
        /* It's me! Geez! */
        channel_remove(env, ev->m_params[0]);
    }
    else
    {
        /* Remove user from channel! */
        channel_remove_user(env, ev->m_params[0], ev->m_params[1]);
    }

    return 0;
}

int handle_unknown(luna_state *env, irc_message *ev)
{
    return 0;
}

int handle_server_supports(luna_state *env, irc_message *ev)
{
    int i;

    for (i = 1; i < ev->m_paramcount; ++i)
    {
        char *key = strtok(ev->m_params[i], "=");
        char *val = strtok(NULL, "");

        if (!strcasecmp(key, "CHANMODES"))
        {
            char *tok = strtok(val, ",");
            strncpy(env->chanmodes.param_address, tok,
                    sizeof(env->chanmodes.param_address) - 1);

            tok = strtok(NULL, ",");
            strncpy(env->chanmodes.param_always, tok,
                    sizeof(env->chanmodes.param_always) - 1);

            tok = strtok(NULL, ",");
            strncpy(env->chanmodes.param_whenset, tok,
                    sizeof(env->chanmodes.param_whenset) - 1);

            tok = strtok(NULL, ",");
            strncpy(env->chanmodes.param_never, tok,
                    sizeof(env->chanmodes.param_never) - 1);
        }
        else if (!strcasecmp(key, "PREFIX"))
        {
            /*PREFIX=(ov)@+ */
            int k;
            int max = sizeof(env->userprefix) / sizeof(env->userprefix[0]);

            for (k = 0; (k < strlen(val) / 2 - 1) && (k < max); ++k)
            {
                env->userprefix[k].mode = *(val + k + 1);
                env->userprefix[k].prefix = *(val + k + strlen(val) / 2 + 1);

                channel_modes *m = &(env->chanmodes);

                if (strlen(m->param_nick) < sizeof(m->param_nick) - 1)
                {
                    int ind = strlen(m->param_nick);

                    m->param_nick[ind] = *(val + k + 1);
                    m->param_nick[ind + 1] = 0;
                }
            }
        }
        else if (!strcasecmp(key, "CHANTYPES"))
        {
            env->chantypes = xstrdup(val);
        }
    }

    return 0;
}

int handle_mode_change(luna_state *state, const char *channel,
                       const char *flags, char **args, int argind)
{
    int action = 0; /* 0 = set, 1 = unset */
    int i = argind;

    irc_channel *target = channel_get(state, channel);

    if (!target)
    {
        logger_log(state->logger, LOGLEV_WARNING, "Unknown channel `%s'",
                   channel);
        return 1;
    }

    while (*flags)
    {
        int max = sizeof(target->flags) / sizeof(target->flags[0]);

        switch (*flags)
        {
        case '+':
            action = 0;
            flags++;
            break;
        case '-':
            action = 1;
            flags++;
            break;
        }

        int flag = *flags - 'A';

        if (flag >= max)
        {
            logger_log(state->logger, LOGLEV_WARNING, "Mode `%c' out of range",
                       *flags);
            return 0;
        }

        /*
         * Do all modes that need an argument
         */
        if  (strchr(state->chanmodes.param_address, *flags) ||
             strchr(state->chanmodes.param_always, *flags) ||
             (strchr(state->chanmodes.param_whenset, *flags) && !action))
        {
            /* Set/Unset flag "*flags" with argument "args[i]" */
            char *arg = args[i++];

            if (!action)
            {
                if (strchr(state->chanmodes.param_address, *flags))
                {
                    /* Add to (or create) list */
                    if (!target->flags[flag].set)
                    {
                        /* Flag not set, set it and create the list */
                        target->flags[flag].set = 1;
                        target->flags[flag].type = FLAG_LIST;
                        list_init(&target->flags[flag].list);
                    }

                    list_push_back(target->flags[flag].list, xstrdup(arg));
                }
                else
                {
                    target->flags[flag].set = 1;
                    target->flags[flag].type = FLAG_STRING;
                    target->flags[flag].string = xstrdup(arg);
                }
            }
            else
            {
                /* Remove from list, possibly remove list, too */
                if (target->flags[flag].set)
                {
                    if (strchr(state->chanmodes.param_address, *flags))
                    {
                        char *entry = (char *)list_find(
                                          target->flags[flag].list,
                                          arg,
                                          (list_find_fn)&strcasecmp);

                        if (entry)
                        {
                            list_delete(target->flags[flag].list, entry,
                                        &mm_free);
                        }

                        if (target->flags[flag].list->length == 0)
                        {
                            list_destroy(target->flags[flag].list, &mm_free);
                            target->flags[flag].type = FLAG_NONE;
                            target->flags[flag].set = 0;
                        }
                    }
                    else
                    {
                        target->flags[flag].set = 0;
                        mm_free(target->flags[flag].string);
                        target->flags[flag].string = NULL;
                        target->flags[flag].type = FLAG_NONE;
                    }
                }
            }
        }
        else if (strchr(state->chanmodes.param_nick, *flags))
        {
            const char *arg = args[i++];

            irc_user *user = channel_get_user(state, channel, arg);

            if (user)
            {
                if (!action)
                {
                    if ((strlen(user->modes) < sizeof(user->modes))
                            && (!strchr(user->modes, *flags)))
                    {
                        int len = strlen(user->modes);
                        user->modes[len] = *flags;
                        user->modes[len + 1] = 0;
                    }
                }
                else
                {
                    const char *oldmodes = user->modes;
                    memset(user->modes, 0, sizeof(user->modes));

                    while (*oldmodes)
                    {
                        if (*oldmodes != *flags)
                        {
                            int len = strlen(user->modes);
                            user->modes[len] = *oldmodes;
                            user->modes[len + 1] = 0;
                        }

                        oldmodes++;
                    }
                }
            }
            else
            {
                logger_log(state->logger, LOGLEV_WARNING,
                           "Tried to alter unknown user `%s'", arg);
            }
        }
        else
        {
            /* Unknown flags and whenset-parameters */
            /* Unknown flags are treated as if they require no parameter */

            /* Set/Unset flag "*flags" for channel */
            if (!action)
            {
                target->flags[flag].set = 1;
            }
            else
            {
                target->flags[flag].set = 0;

                if (target->flags[flag].type == FLAG_STRING)
                    mm_free(target->flags[flag].string);

                target->flags[flag].type = FLAG_NONE;
            }
        }

        flags++;
    }

    return 0;
}
