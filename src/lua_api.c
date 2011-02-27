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
static int api_get_user_level(lua_State *);

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
  /*{ "signal_remove",   api_signal_remove },*/ /* Meh */
    { "get_user_level",  api_get_user_level },

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
api_push_sender(lua_State *L, irc_sender *s)
{
    int table_index;

    lua_newtable(L);
    table_index = lua_gettop(L);

    /* Push nick */
    lua_pushstring(L, "nick");
    lua_pushstring(L, s->nick);
    lua_settable(L, table_index);

    /* Push user */
    lua_pushstring(L, "user");
    lua_pushstring(L, s->user);
    lua_settable(L, table_index);

    /* Push host */
    lua_pushstring(L, "host");
    lua_pushstring(L, s->host);
    lua_settable(L, table_index);

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
    if (((lua_type(L, n) == LUA_TSTRING) && (strcmp(name, "")))        &&
        ((lua_type(L, d) == LUA_TSTRING) && (strcmp(descr, ""))) &&
        ((lua_type(L, a) == LUA_TSTRING) && (strcmp(author, "")))      &&
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
    int n = lua_gettop(L);
    int api_table;
    int callbacks_array;
    int callback_table;
    int len;

    if (n != 2)
        return luaL_error(L, "expected 2 arguments, got %d", n);

    if ((lua_type(L, 1) != LUA_TSTRING) || (lua_type(L, 2) != LUA_TFUNCTION))
        return luaL_error(L, "expected string and function");

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
    int n = lua_gettop(L);
    int api_table;
    int info_table;

    if (n != 4)
        return luaL_error(L, "expected 4 arguments, got %d", n);

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
api_get_user_level(lua_State *L)
{
    int n = lua_gettop(L);
    int u;
    int h;

    luna_state *state = NULL;

    const char *nick = NULL;
    const char *user = NULL;
    const char *host = NULL;

    if (n != 1)
        return luaL_error(L, "expected 1 argument, got %d", n);

    if (lua_type(L, 1) != LUA_TTABLE)
        return luaL_error(L, "expected table, got %s",
                          lua_typename(L, lua_type(L, 1)));

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
        char *level = NULL;

        strncpy(tmp.nick, nick, sizeof(tmp.nick) - 1);
        strncpy(tmp.user, user, sizeof(tmp.user) - 1);
        strncpy(tmp.host, host, sizeof(tmp.host) - 1);

        if ((level = user_get_level(state->users, &tmp)) != NULL)
            lua_pushstring(L, level);
        else
            lua_pushnil(L);

        return 1;
    }
    else
        return luaL_error(L, "user table not valid");
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
    int n = lua_gettop(L);

    if (n != 1)
        return luaL_error(L, "expected 1 argument, got %d", n);

    lua_pushnumber(L, api_command_helper(L, lua_tostring(L, 1)));
    return 1;
}


/* TODO: Have a smarter way for all the following repeating */
static int
api_privmsg(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if (n != 2)
        return luaL_error(L, "expected 2 arguments, got %d", n);

    ret = api_command_helper(L, "PRIVMSG %s :%s",
                             lua_tostring(L, 1), lua_tostring(L, 2));

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_notice(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if (n != 2)
        return luaL_error(L, "expected 2 arguments, got %d", n);

    ret = api_command_helper(L, "NOTICE %s :%s",
                      lua_tostring(L, 1), lua_tostring(L, 2));

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_join(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if ((n < 1) || (n > 2))
        return luaL_error(L, "expected 1 or 2 arguments, got %d", n);

    if (n == 2)
        ret = api_command_helper(L, "JOIN %s :%s",
                                 lua_tostring(L, 1), lua_tostring(L, 2));
    else
        ret = api_command_helper(L, "JOIN %s", lua_tostring(L, 1));

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_part(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if ((n < 1) || (n > 2))
        return luaL_error(L, "expected 1 or 2 arguments, got %d", n);

    if (n == 2)
        ret = api_command_helper(L, "PART %s :%s",
                                 lua_tostring(L, 1), lua_tostring(L, 2));
    else
        ret = api_command_helper(L, "PART %s", lua_tostring(L, 1));

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_quit(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if (n > 1)
        return luaL_error(L, "expected 0 or 1 argument, got %d", n);

    if (n == 1)
        ret = api_command_helper(L, "QUIT :%s", lua_tostring(L, 1));
    else
        ret = api_command_helper(L, "QUIT");

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_change_nick(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if (n != 1)
        return luaL_error(L, "expected 1 argument, got %d", n);

    ret = api_command_helper(L, "NICK :%s", lua_tostring(L, 1));

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_kick(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if (n != 3)
        return luaL_error(L, "expected 3 arguments, got %d", n);

    ret = api_command_helper(L, "KICK %s %s :%s",
            lua_tostring(L, 1), lua_tostring(L, 2), lua_tostring(L, 3));

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_set_modes(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if (n != 2)
        return luaL_error(L, "expected 2 arguments, got %d", n);

    ret = api_command_helper(L, "MODE %s %s",
            lua_tostring(L, 1), lua_tostring(L, 2));

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_set_topic(lua_State *L)
{
    int n = lua_gettop(L);
    int ret;

    if (n != 2)
        return luaL_error(L, "expected 2 arguments, got %d", n);

    ret = api_command_helper(L, "TOPIC %s :%s",
            lua_tostring(L, 1), lua_tostring(L, 2));

    lua_pushnumber(L, ret);
    return 1;
}


static int
api_scripts(lua_State *L)
{
    int n = lua_gettop(L);
    int array;
    int i = 1;
    luna_state *state;
    list_node *cur = NULL;

    if (n != 0)
        return luaL_error(L, "expected no arguments, got %d", n);

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

    lua_pushstring(L, "name");
    lua_pushstring(L, script->name);
    lua_settable(L, table);

    lua_pushstring(L, "description");
    lua_pushstring(L, script->description);
    lua_settable(L, table);

    lua_pushstring(L, "version");
    lua_pushstring(L, script->version);
    lua_settable(L, table);

    lua_pushstring(L, "author");
    lua_pushstring(L, script->author);
    lua_settable(L, table);

    lua_pushstring(L, "file");
    lua_pushstring(L, script->filename);
    lua_settable(L, table);

    return 0;
}


int
api_push_channel(lua_State *L, irc_channel *channel)
{
    int table;
    int array;
    int i = 1;
    list_node *cur = NULL;

    lua_newtable(L);
    table = lua_gettop(L);

    lua_pushstring(L, "name");
    lua_pushstring(L, channel->name);
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

    lua_pushstring(L, "nick");
    lua_pushstring(L, user->nick);
    lua_settable(L, table);

    lua_pushstring(L, "user");
    lua_pushstring(L, user->user);
    lua_settable(L, table);

    lua_pushstring(L, "host");
    lua_pushstring(L, user->host);
    lua_settable(L, table);

    return 0;
}


int
api_push_luna_user(lua_State *L, luna_user *user)
{
    int table;

    lua_newtable(L);
    table = lua_gettop(L);

    lua_pushstring(L, "hostmask");
    lua_pushstring(L, user->hostmask);
    lua_settable(L, table);

    lua_pushstring(L, "level");
    lua_pushstring(L, user->level);
    lua_settable(L, table);

    return 0;
}


static int
api_channels(lua_State *L)
{
    int n = lua_gettop(L);
    int array;
    int i = 1;
    luna_state *state;
    list_node *cur = NULL;

    if (n != 0)
        return luaL_error(L, "expected no arguments, got %d", n);

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
    int n = lua_gettop(L);
    int table;
    int user_table;
    int server_table;

    luna_state *state;

    if (n != 0)
        return luaL_error(L, "expected no arguments, got %d", n);

    state = api_getstate(L);

    lua_newtable(L);
    table = lua_gettop(L);

    lua_pushstring(L, "identity");
    lua_newtable(L);
    user_table = lua_gettop(L);

    lua_pushstring(L, "nick");
    lua_pushstring(L, state->userinfo.nick);
    lua_settable(L, user_table);

    lua_pushstring(L, "user");
    lua_pushstring(L, state->userinfo.user);
    lua_settable(L, user_table);

    lua_pushstring(L, "real");
    lua_pushstring(L, state->userinfo.real);
    lua_settable(L, user_table);
    lua_settable(L, table);

    lua_pushstring(L, "server");
    lua_newtable(L);
    server_table = lua_gettop(L);

    lua_pushstring(L, "host");
    lua_pushstring(L, state->serverinfo.host);
    lua_settable(L, server_table);

    lua_pushstring(L, "port");
    lua_pushnumber(L, state->serverinfo.port);
    lua_settable(L, server_table);
    lua_settable(L, table);

    lua_pushstring(L, "started");
    lua_pushnumber(L, state->started);
    lua_settable(L, table);

    lua_pushstring(L, "connected");
    lua_pushnumber(L, state->connected);
    lua_settable(L, table);

    return 1;
}


static int
api_users(lua_State *L)
{
    int n = lua_gettop(L);
    int array;
    int i = 1;
    luna_state *state;
    list_node *cur = NULL;

    if (n != 0)
        return luaL_error(L, "expected no arguments, got %d", n);

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
