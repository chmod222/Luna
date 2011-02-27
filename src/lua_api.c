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
 *  Lua script management (lua_api.c)
 *  ---
 *  Load, unload and emit signals to Lua scripts
 *
 *  Created: 25.02.2011 20:26:13
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "lua_api.h"
#include "state.h"
#include "logger.h"
#include "channel.h"
#include "user.h"
#include "irc.h"


static int api_script_register(lua_State *);
static int api_signal_add(lua_State *);
static int api_get_user(lua_State *);
static int api_add_user(lua_State *);
static int api_remove_user(lua_State *);
static int api_reload_userlist(lua_State *);
static int api_load_script(lua_State *);
static int api_unload_script(lua_State *);
static int api_log(lua_State *);

static int api_sendline(lua_State *);
static int api_privmsg(lua_State *);
static int api_notice(lua_State *);
static int api_join(lua_State *);
static int api_part(lua_State *);
static int api_quit(lua_State *);
static int api_change_nick(lua_State *);
static int api_kick(lua_State *);
static int api_set_modes(lua_State *);
static int api_set_topic(lua_State *);

static int api_channels(lua_State *);
static int api_scripts(lua_State *);
static int api_info(lua_State *);
static int api_users(lua_State *);

static char *env_key = "Env";
static luaL_Reg api_library[] = {
    { "script_register", api_script_register },
    { "signal_add",      api_signal_add },
    { "get_user",        api_get_user },

    { "add_user",        api_add_user },
    { "remove_user",     api_remove_user },
    { "reload_userlist", api_reload_userlist },
    { "load_script",     api_load_script },
    { "unload_script",   api_unload_script },
    { "log",             api_log },

    { "sendline",        api_sendline },
    { "privmsg",         api_privmsg },
    { "notice",          api_notice },
    { "join_channel",    api_join },
    { "part_channel",    api_part },
    { "quit",            api_quit },
    { "change_nick",     api_change_nick },
    { "kick",            api_kick },
    { "set_modes",       api_set_modes },
    { "set_topic",       api_set_topic },

    { "channels",        api_channels },
    { "scripts",         api_scripts },
    { "info",            api_info },
    { "users",           api_users },

    { NULL, NULL }
};


int
script_cmp(void *data, void *list_data)
{
    char *name = (char *)data;
    luna_script *script = (luna_script *)list_data;

    return strcmp(name, script->filename);
}


void
script_free(void *list_data)
{
    luna_script *script = (luna_script *)list_data;

    /* TODO: Emit "unload" signal */
    lua_close(script->state);
    free(list_data);

    return;
}


int
script_load(luna_state *state, const char *file)
{
    lua_State *L;
    luna_script *script;

    /* Script can be openened and allocated? */
    if ((script = malloc(sizeof(*script))) == NULL)
        return 1;

    if ((L = lua_open()) == NULL)
    {
        free(script);

        return 1;
    }

    memset(script, 0, sizeof(*script));
    script->state = L;

    luaL_openlibs(L);

    /* Push Luna state pointer to script so it has access to data */
    lua_pushlightuserdata(L, (void *)env_key);
    lua_pushlightuserdata(L, state);
    lua_settable(L, LUA_REGISTRYINDEX);

    /* Register library methods */
    luaL_register(L, LIBNAME, api_library);

    /* Register empty callbacks table */
    lua_getglobal(L, LIBNAME);
    lua_pushstring(L, "__callbacks");
    lua_newtable(L);
    lua_settable(L, -3);

    api_push_loglevels(L);

    /* Clean the stack */
    lua_pop(L, -1);

    /* Execute */
    if ((luaL_dofile(L, file) == 0) && (script_identify(L, script) == 0))
    {
        list_push_back(state->scripts, script);
        strncpy(script->filename, file, sizeof(script->filename) - 1);

        script_emit(state, script, "load", NULL, NULL);

        return 0;
    }
    else
    {
        /* luaL_dofile() failure or invalid script */
        logger_log(state->logger, LOGLEV_ERROR, "Lua error: %s",
                   lua_tostring(L, -1));

        free(script);
        lua_close(L);
    }

    return 1;
}


int
script_unload(luna_state *state, const char *file)
{
    void *key = (void *)file;
    void *result = list_find(state->scripts, key, &script_cmp);

    if (result)
    {
        /* Call unload signal */
        script_emit(state, (luna_script *)result, "unload", NULL, NULL);
        list_delete(state->scripts, result, &script_free);

        return 0;
    }

    return 1;
}


int
api_setfield_s(lua_State *L, int table, const char *key, const char *value)
{
    lua_pushstring(L, key);
    lua_pushstring(L, value);
    lua_settable(L, table);

    return 0;
}


int
api_setfield_n(lua_State *L, int table, const char *key, double value)
{
    lua_pushstring(L, key);
    lua_pushnumber(L, value);
    lua_settable(L, table);

    return 0;
}


int
api_push_loglevels(lua_State *L)
{
    int api_table;
    int level_table;

    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    lua_pushstring(L, "log_level");
    lua_newtable(L);
    level_table = lua_gettop(L);

    api_setfield_n(L, level_table, "info", LOGLEV_INFO);
    api_setfield_n(L, level_table, "warning", LOGLEV_WARNING);
    api_setfield_n(L, level_table, "error", LOGLEV_ERROR);

    lua_settable(L, api_table);

    return 0;
}


int
api_push_sender(lua_State *L, irc_sender *s)
{
    int table_index;

    lua_newtable(L);
    table_index = lua_gettop(L);

    api_setfield_s(L, table_index, "nick", s->nick);
    api_setfield_s(L, table_index, "user", s->user);
    api_setfield_s(L, table_index, "host", s->host);

    return 0;
}


int
signal_dispatch(luna_state *state, const char *sig, const char *fmt, ...)
{
    va_list args;
    list_node *cur;

    for (cur = state->scripts->root; cur != NULL; cur = cur->next)
    {
        va_start(args, fmt);
        script_emit(state, cur->data, sig, fmt, args);
        va_end(args);
    }

    return 0;
}


int
script_emit(luna_state *state, luna_script *script, const char *sig,
            const char *fmt, va_list args)
{
    int api_table;
    int callbacks_array;
    int len;
    int i = 1;

    lua_State *L = script->state;

    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    lua_pushstring(L, "__callbacks");
    lua_gettable(L, api_table);
    callbacks_array = lua_gettop(L);
    len = lua_objlen(L, callbacks_array);

    while (i <= len)
    {
        int callback_table;
        int j = 0;

        lua_rawgeti(L, callbacks_array, i);
        callback_table = lua_gettop(L);

        lua_pushstring(L, "signal");
        lua_gettable(L, callback_table);

        if (!strcmp(lua_tostring(L, -1), sig))
        {
            /* Emit */

            /* Push function to stack */
            lua_pushstring(L, "callback");
            lua_gettable(L, callback_table);

            if (fmt != NULL)
            {
                for (j = 0; fmt[j] != 0; ++j)
                {
                    switch (fmt[j])
                    {
                        case 's':
                            lua_pushstring(L, va_arg(args, char *));
                            break;

                        case 'p':
                            api_push_sender(L, va_arg(args, irc_sender *));
                            break;

                        case 'n':
                            lua_pushnumber(L, va_arg(args, double));
                            break;

                        default:
                            j--;
                            break;
                    }
                }
            }

            if (lua_pcall(L, j, 0, 0) != 0)
                logger_log(state->logger, LOGLEV_ERROR,
                           "Lua error ('%s@%s'): %s",
                           sig, script->filename, lua_tostring(L, -1));

            lua_pop(L, -1);
            return 0;
        }

        lua_pop(L, 2);
        ++i;
    }

    lua_pop(L, -1);
    return 1;
}


int
script_identify(lua_State *L, luna_script *script)
{
    int api_table;
    int info_table;

    /* Indices */
    int n; /* Name */
    int d; /* Description */
    int a; /* Author */
    int v; /* Version */

    /* Strings */
    const char *name = NULL;
    const char *descr = NULL;
    const char *author = NULL;
    const char *version = NULL;

    /* Get API table */
    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    /* Push key of scriptinfo table and get it's value */
    lua_pushstring(L, "__scriptinfo");
    lua_gettable(L, api_table);
    info_table = lua_gettop(L);

    /* Is it defined and a table? */
    if (lua_type(L, info_table) != LUA_TTABLE)
    {
        lua_pop(L, -1);
        return 1;
    }

    lua_pushstring(L, "name");
    lua_gettable(L, info_table);
    n = lua_gettop(L);

    lua_pushstring(L, "description");
    lua_gettable(L, info_table);
    d = lua_gettop(L);

    lua_pushstring(L, "version");
    lua_gettable(L, info_table);
    v = lua_gettop(L);

    lua_pushstring(L, "author");
    lua_gettable(L, info_table);
    a = lua_gettop(L);

    name = lua_tostring(L, n);
    descr = lua_tostring(L, d);
    author = lua_tostring(L, a);
    version = lua_tostring(L, v);

    /* Values must be strings and nonempty */
    if (((lua_type(L, n) == LUA_TSTRING) && (strcmp(name, "")))    &&
        ((lua_type(L, d) == LUA_TSTRING) && (strcmp(descr, "")))   &&
        ((lua_type(L, a) == LUA_TSTRING) && (strcmp(author, "")))  &&
        ((lua_type(L, v) == LUA_TSTRING) && (strcmp(version, ""))))
    {
        /* Copy values over */
        strncpy(script->name, name, sizeof(script->name) - 1);
        strncpy(script->description, descr, sizeof(script->description) - 1);
        strncpy(script->version, version, sizeof(script->version) - 1);
        strncpy(script->author, author, sizeof(script->author) - 1);

        lua_pop(L, -1);
        return 0;
    }
    else
    {
        lua_pop(L, -1);
        return 1;
    }
}


static int
api_signal_add(lua_State *L)
{
    int api_table;
    int callbacks_array;
    int callback_table;
    int len;

    luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    lua_pushstring(L, "__callbacks");
    lua_gettable(L, api_table);
    callbacks_array = lua_gettop(L);
    len = lua_objlen(L, callbacks_array);

    lua_newtable(L);
    callback_table = lua_gettop(L);

    lua_pushstring(L, "signal");
    lua_pushvalue(L, 1);
    lua_settable(L, callback_table);

    lua_pushstring(L, "callback");
    lua_pushvalue(L, 2);
    lua_settable(L, callback_table);

    lua_rawseti(L, callbacks_array, len + 1);

    lua_pop(L, -1);
    return 0;
}


static int
api_script_register(lua_State *L)
{
    int api_table;
    int info_table;

    luaL_checkstring(L, 1);
    luaL_checkstring(L, 2);
    luaL_checkstring(L, 3);
    luaL_checkstring(L, 4);

    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    lua_pushstring(L, "__scriptinfo");
    lua_newtable(L);
    info_table = lua_gettop(L);

    /* Fill new table */
    lua_pushstring(L, "name");
    lua_pushvalue(L, 1); /* Argument 1 of luna.script_register() */
    lua_settable(L, info_table);

    lua_pushstring(L, "description");
    lua_pushvalue(L, 2); /* Argument 2 of luna.script_register() */
    lua_settable(L, info_table);

    lua_pushstring(L, "version");
    lua_pushvalue(L, 3); /* Argument 3 of luna.script_register() */
    lua_settable(L, info_table);

    lua_pushstring(L, "author");
    lua_pushvalue(L, 4); /* Argument 4 of luna.script_register() */
    lua_settable(L, info_table);

    /* Set new table to LIBNAME.__scriptinfo */
    lua_settable(L, api_table);

    lua_pop(L, -1);
    return 0;
}


static int
api_get_user(lua_State *L)
{
    int n = lua_gettop(L);
    int u;
    int h;

    luna_state *state = NULL;

    const char *nick = NULL;
    const char *user = NULL;
    const char *host = NULL;

    luaL_checktype(L, 1, LUA_TTABLE);

    state = api_getstate(L);

    lua_pushstring(L, "nick");
    lua_gettable(L, 1);
    n = lua_gettop(L); /* Variable reuse for great good */

    lua_pushstring(L, "user");
    lua_gettable(L, 1);
    u = lua_gettop(L);

    lua_pushstring(L, "host");
    lua_gettable(L, 1);
    h = lua_gettop(L);

    nick = lua_tostring(L, n);
    user = lua_tostring(L, u);
    host = lua_tostring(L, h);

    if (((lua_type(L, n) == LUA_TSTRING) && (strcmp(nick, ""))) &&
        ((lua_type(L, u) == LUA_TSTRING) && (strcmp(user, ""))) &&
        ((lua_type(L, h) == LUA_TSTRING) && (strcmp(host, ""))))
    {
        /* Copy values over */
        irc_sender tmp;
        void *data = NULL;
        void *key = (void *)&tmp;

        strncpy(tmp.nick, nick, sizeof(tmp.nick) - 1);
        strncpy(tmp.user, user, sizeof(tmp.user) - 1);
        strncpy(tmp.host, host, sizeof(tmp.host) - 1);

        if ((data = list_find(state->users, key, &luna_user_cmp)) != NULL)
        {
            luna_user *u = (luna_user *)data;
            int table;

            lua_newtable(L);
            table = lua_gettop(L);

            lua_pushstring(L, "hostmask");
            lua_pushstring(L, u->hostmask);
            lua_settable(L, table);

            lua_pushstring(L, "level");
            lua_pushstring(L, u->level);
            lua_settable(L, table);
        }
        else
        {
            lua_pushnil(L);
        }

        return 1;
    }
    else
        return luaL_error(L, "user table not valid");
}


static int
api_add_user(lua_State *L)
{
    const char *mask  = luaL_checkstring(L, 1);
    const char *level = luaL_checkstring(L, 2);

    luna_state *state = api_getstate(L);

    users_add(state->users, mask, level);
    users_write(state->users, "users.txt");

    return 0;
}


static int
api_remove_user(lua_State *L)
{
    const char *mask = luaL_checkstring(L, 1);

    luna_state *state = api_getstate(L);

    users_remove(state->users, mask);
    users_write(state->users, "users.txt");

    return 0;
}


static int
api_reload_userlist(lua_State *L)
{
    luna_state *state = api_getstate(L);

    users_reload(state->users, "users.txt");

    return 0;
}


static int
api_load_script(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    luna_state *state = api_getstate(L);

    if (!script_load(state, file))
    {
        void *script = list_find(state->scripts, (void *)file, &script_cmp);
        luna_script *s = (luna_script *)script;

        api_push_script(L, s);
        return 1;
    }
    else
    {
        return 0;
    }
}


static int
api_unload_script(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    luna_state *state = api_getstate(L);

    script_unload(state, file);

    return 0;
}


luna_state *
api_getstate(lua_State *L)
{
    luna_state *state = NULL;
    int top = lua_gettop(L);

    lua_pushlightuserdata(L, (void *)env_key);
    lua_gettable(L, LUA_REGISTRYINDEX);

    state = lua_touserdata(L, top + 1);
    lua_pop(L, 1);

    return state;
}


static int
api_log(lua_State *L)
{
    int level = luaL_checknumber(L, 1);
    const char *message = luaL_checkstring(L, 2);
    luna_state *state = api_getstate(L);

    lua_pushnumber(L, logger_log(state->logger, level, "%s", message));

    return 1;
}


int
api_command_helper(lua_State *L, const char *fmt, ...)
{
    va_list args;
    int ret;
    luna_state *state = api_getstate(L);

    va_start(args, fmt);
    ret = net_vsendfln(state, fmt, args);
    va_end(args);

    return ret;
}


static int
api_sendline(lua_State *L)
{
    const char *line = luaL_checkstring(L, 1);

    lua_pushnumber(L, api_command_helper(L, line));
    return 1;
}


/* TODO: Have a smarter way for all the following repeating */
static int
api_privmsg(lua_State *L)
{
    const char *target = luaL_checkstring(L, 1);
    const char *msg    = luaL_checkstring(L, 2);
    int ret;

    ret = api_command_helper(L, "PRIVMSG %s :%s", target, msg);

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_notice(lua_State *L)
{
    const char *target = luaL_checkstring(L, 1);
    const char *msg    = luaL_checkstring(L, 2);
    int ret;

    ret = api_command_helper(L, "NOTICE %s :%s", target, msg);

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_join(lua_State *L)
{
    const char *channel = luaL_checkstring(L, 1);
    int ret;

    if (lua_gettop(L) > 1)
    {
        const char *key = luaL_checkstring(L, 2);

        ret = api_command_helper(L, "JOIN %s :%s", channel, key);
    }
    else
    {
        ret = api_command_helper(L, "JOIN %s", channel);
    }

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_part(lua_State *L)
{
    const char *channel = luaL_checkstring(L, 1);
    int ret;

    if (lua_gettop(L) > 1)
    {
        const char *reason = luaL_checkstring(L, 2);

        ret = api_command_helper(L, "PART %s :%s", channel, reason);
    }
    else
    {
        ret = api_command_helper(L, "PART %s", channel);
    }

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_quit(lua_State *L)
{
    int ret;

    if (lua_gettop(L) > 0)
    {
        const char *reason = luaL_checkstring(L, 1);

        ret = api_command_helper(L, "QUIT :%s", reason);
    }
    else
    {
        ret = api_command_helper(L, "QUIT");
    }

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_change_nick(lua_State *L)
{
    const char *newnick = luaL_checkstring(L, 1);
    int ret;

    ret = api_command_helper(L, "NICK :%s", newnick);

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_kick(lua_State *L)
{
    const char *channel = luaL_checkstring(L, 1);
    const char *user    = luaL_checkstring(L, 2);
    const char *reason  = luaL_checkstring(L, 3);
    int ret;

    ret = api_command_helper(L, "KICK %s %s :%s", channel, user, reason);

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_set_modes(lua_State *L)
{
    const char *target = luaL_checkstring(L, 1);
    const char *args   = luaL_checkstring(L, 2);
    int ret;

    ret = api_command_helper(L, "MODE %s %s", target, args);

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_set_topic(lua_State *L)
{
    const char *channel  = luaL_checkstring(L, 1);
    const char *newtopic = luaL_checkstring(L, 2);
    int ret;

    ret = api_command_helper(L, "TOPIC %s :%s", channel, newtopic);

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_scripts(lua_State *L)
{
    int array;
    int i = 1;
    luna_state *state;
    list_node *cur = NULL;

    state = api_getstate(L);

    lua_newtable(L);
    array = lua_gettop(L);

    for (cur = state->scripts->root; cur != NULL; cur = cur->next)
    {
        api_push_script(L, cur->data);

        lua_rawseti(L, array, i++);
    }

    return 1;
}


int
api_push_script(lua_State *L, luna_script *script)
{
    int table;

    lua_newtable(L);
    table = lua_gettop(L);

    api_setfield_s(L, table, "name", script->name);
    api_setfield_s(L, table, "description", script->description);
    api_setfield_s(L, table, "version", script->version);
    api_setfield_s(L, table, "author", script->author);
    api_setfield_s(L, table, "file", script->filename);

    return 0;
}


int
api_push_channel(lua_State *L, irc_channel *channel)
{
    int table;
    int array;
    int topic_table;
    int i = 1;
    list_node *cur = NULL;

    lua_newtable(L);
    table = lua_gettop(L);

    api_setfield_s(L, table, "name", channel->name);

    lua_pushstring(L, "topic");
    lua_newtable(L);
    topic_table = lua_gettop(L);

    api_setfield_s(L, topic_table, "text", channel->topic);
    api_setfield_s(L, topic_table, "set_by", channel->topic_setter);
    api_setfield_n(L, topic_table, "set_at", channel->topic_set);

    lua_settable(L, table);


    lua_pushstring(L, "users");
    lua_newtable(L);
    array = lua_gettop(L);

    for (cur = channel->users->root; cur != NULL; cur = cur->next)
    {
        api_push_user(L, cur->data);

        lua_rawseti(L, array, i++);
    }

    lua_settable(L, table);

    return 0;
}


int
api_push_user(lua_State *L, irc_user *user)
{
    int table;

    lua_newtable(L);
    table = lua_gettop(L);

    api_setfield_s(L, table, "nick", user->nick);
    api_setfield_s(L, table, "user", user->user);
    api_setfield_s(L, table, "host", user->host);

    return 0;
}


int
api_push_luna_user(lua_State *L, luna_user *user)
{
    int table;

    lua_newtable(L);
    table = lua_gettop(L);

    api_setfield_s(L, table, "hostmask", user->hostmask);
    api_setfield_s(L, table, "level", user->level);

    return 0;
}


static int
api_channels(lua_State *L)
{
    int array;
    int i = 1;
    luna_state *state;
    list_node *cur = NULL;

    state = api_getstate(L);

    lua_newtable(L);
    array = lua_gettop(L);

    for (cur = state->channels->root; cur != NULL; cur = cur->next)
    {
        api_push_channel(L, cur->data);

        lua_rawseti(L, array, i++);
    }

    return 1;
}


static int
api_info(lua_State *L)
{
    int table;
    int user_table;
    int server_table;

    luna_state *state;

    state = api_getstate(L);

    lua_newtable(L);
    table = lua_gettop(L);

    lua_pushstring(L, "identity");
    lua_newtable(L);
    user_table = lua_gettop(L);

    api_setfield_s(L, user_table, "nick", state->userinfo.nick);
    api_setfield_s(L, user_table, "user", state->userinfo.user);
    api_setfield_s(L, user_table, "real", state->userinfo.real);

    lua_settable(L, table);

    lua_pushstring(L, "server");
    lua_newtable(L);
    server_table = lua_gettop(L);

    api_setfield_s(L, server_table, "host", state->serverinfo.host);
    api_setfield_n(L, server_table, "port", state->serverinfo.port);

    lua_settable(L, table);

    api_setfield_n(L, table, "started", state->started);
    api_setfield_n(L, table, "connected", state->connected);

    return 1;
}


static int
api_users(lua_State *L)
{
    int array;
    int i = 1;
    luna_state *state;
    list_node *cur = NULL;

    state = api_getstate(L);

    lua_newtable(L);
    array = lua_gettop(L);

    for (cur = state->users->root; cur != NULL; cur = cur->next)
    {
        api_push_luna_user(L, cur->data);

        lua_rawseti(L, array, i++);
    }

    return 1;
}
