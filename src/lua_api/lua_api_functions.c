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
 *  Lua api functions (lua_api_functions.c)
 *  ---
 *  Functions to register in each script environment
 *
 *  Created: 03.02.2012 02:25:34
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "lua_manager.h"
#include "lua_api_functions.h"
#include "lua_util.h"

luaL_Reg api_library[] = {
    { "script_register", api_script_register },
    { "signal_add",      api_signal_add },

    { "add_user",        api_add_user },
    { "remove_user",     api_remove_user },
    { "reload_userlist", api_reload_userlist },

    { "get_user",        api_get_user },
    { "set_user_flags",  api_set_user_flags },
    { "set_user_level",  api_set_user_level },
    { "set_user_id",     api_set_user_id },

    { "user_flag_set",   api_user_flag_set },

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
api_script_register(lua_State *L)
{
    int arg = 1;
    const char *name = NULL;
    const char *author = NULL;
    const char *descr = NULL;
    const char *version = NULL;

    luaL_checktype(L, arg, LUA_TTABLE);

    lua_pushstring(L, "name");
    name = lua_tostring(L, (lua_gettable(L, arg), lua_gettop(L)));

    lua_pushstring(L, "author");
    author = lua_tostring(L, (lua_gettable(L, arg), lua_gettop(L)));

    lua_pushstring(L, "description");
    descr = lua_tostring(L, (lua_gettable(L, arg), lua_gettop(L)));

    lua_pushstring(L, "version");
    version = lua_tostring(L, (lua_gettable(L, arg), lua_gettop(L)));

    if ((name && strcmp(name, "")) && (author && strcmp(author, "")) &&
        (descr && strcmp(descr, "")) && (version && strcmp(version, "")))
    {
        lua_getglobal(L, LIBNAME);
        lua_pushstring(L, "__scriptinfo");
        lua_pushvalue(L, 1);
        lua_settable(L, -3);
    }
    else
    {
        return luaL_error(L, "fields 'name', 'author', 'description' and "
                             "'version' must not be empty");
    }

    lua_pop(L, -1);
    return 0;
}


int
api_signal_add(lua_State *L)
{
    int api_table;
    int callbacks_array;
    int callback_table;
    int len;

    luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    /* luna,__callbacks[#luna.__callbacks + 1] = {signal = 1, callback = 2} */
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


int
api_get_user(lua_State *L)
{
    luna_state *state = NULL;

    const char *nick = NULL;
    const char *user = NULL;
    const char *host = NULL;

    luaL_checktype(L, 1, LUA_TTABLE);

    state = api_getstate(L);

    lua_pushstring(L, "nick");
    lua_gettable(L, 1);
    nick = lua_tostring(L, lua_gettop(L));

    lua_pushstring(L, "user");
    lua_gettable(L, 1);
    user = lua_tostring(L, lua_gettop(L));

    lua_pushstring(L, "host");
    lua_gettable(L, 1);
    host = lua_tostring(L, lua_gettop(L));

    if ((nick && strcmp(nick, "")) &&
        (user && strcmp(user, "")) &&
        (host && strcmp(host, "")))
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

            api_setfield_s(L, table, "id", u->id);
            api_setfield_s(L, table, "hostmask", u->hostmask);
            api_setfield_s(L, table, "flags", u->flags);
            api_setfield_s(L, table, "level", u->level);
        }
        else
        {
            lua_pushnil(L);
        }

        return 1;
    }
    else
    {
        return luaL_error(L, "user table not valid");
    }
}


int
api_set_user_flags(lua_State *L)
{
    luna_user *user = api_checkuser(L, 1);
    const char *flags = luaL_checkstring(L, 2);

    if (user)
    {
        memset(user->flags, 0, sizeof(user->flags));
        strncpy(user->flags, flags, sizeof(user->flags) - 1);

        api_setfield_s(L, 1, "flags", flags);

        users_write(api_getstate(L), "users.txt");
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}


int
api_set_user_id(lua_State *L)
{
    luna_user *user = api_checkuser(L, 1);
    const char *id = luaL_checkstring(L, 2);

    if (user)
    {
        memset(user->id, 0, sizeof(user->id));
        strncpy(user->id, id, sizeof(user->id) - 1);

        api_setfield_s(L, 1, "id", id);

        users_write(api_getstate(L), "users.txt");
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}


int
api_set_user_level(lua_State *L)
{
    luna_user *user = api_checkuser(L, 1);
    const char *level = luaL_checkstring(L, 2);

    if (user)
    {
        memset(user->level, 0, sizeof(user->level));
        strncpy(user->level, level, sizeof(user->level) - 1);

        api_setfield_s(L, 1, "level", level);

        users_write(api_getstate(L), "users.txt");
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}


int
api_user_flag_set(lua_State *L)
{
    luna_user *user = api_checkuser(L, 1);
    const char *flags = luaL_checkstring(L, 2);

    if (user)
    {
        int i;
        int match = 0;

        for (i = 0; i < strlen(flags); ++i)
        {
            if (strchr(user->flags, flags[i]) != NULL)
            {
                match = 1;
                break;
            }
        }

        if (match)
            lua_pushboolean(L, 1);
        else
            lua_pushboolean(L, 0);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}


int
api_add_user(lua_State *L)
{
    const char *id    = luaL_checkstring(L, 1);
    const char *mask  = luaL_checkstring(L, 2);
    const char *flags = luaL_checkstring(L, 3);
    const char *level = luaL_checkstring(L, 4);

    luna_state *state = api_getstate(L);

    users_add(state, id, mask, flags, level);
    users_write(state, "users.txt");

    return 0;
}


int
api_remove_user(lua_State *L)
{
    const char *mask = luaL_checkstring(L, 1);

    luna_state *state = api_getstate(L);

    users_remove(state, mask);
    users_write(state, "users.txt");

    return 0;
}


int
api_reload_userlist(lua_State *L)
{
    luna_state *state = api_getstate(L);

    users_reload(state, "users.txt");

    return 0;
}


int
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


int
api_unload_script(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    luna_state *state = api_getstate(L);

    script_unload(state, file);

    return 0;
}

int
api_log(lua_State *L)
{
    int level = api_loglevel_from_string(luaL_checkstring(L, 1));
    const char *message = luaL_checkstring(L, 2);
    luna_state *state = api_getstate(L);

    logger_log(state->logger, level, "%s", message);

    return 0;
}

int
api_sendline(lua_State *L)
{
    const char *line = luaL_checkstring(L, 1);

    lua_pushnumber(L, api_command_helper(L, line));

    return 1;
}


/* TODO: Have a smarter way for all the following repeating */
int
api_privmsg(lua_State *L)
{
    const char *target = luaL_checkstring(L, 1);
    const char *msg    = luaL_checkstring(L, 2);
    int ret;

    ret = api_command_helper(L, "PRIVMSG %s :%s", target, msg);

    lua_pushnumber(L, ret);
    return 1;
}


int
api_notice(lua_State *L)
{
    const char *target = luaL_checkstring(L, 1);
    const char *msg    = luaL_checkstring(L, 2);
    int ret;

    ret = api_command_helper(L, "NOTICE %s :%s", target, msg);

    lua_pushnumber(L, ret);
    return 1;
}


int
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


int
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


int
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


int
api_change_nick(lua_State *L)
{
    const char *newnick = luaL_checkstring(L, 1);
    int ret;

    ret = api_command_helper(L, "NICK :%s", newnick);

    lua_pushnumber(L, ret);
    return 1;
}


int
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


int
api_set_modes(lua_State *L)
{
    const char *target = luaL_checkstring(L, 1);
    const char *args   = luaL_checkstring(L, 2);
    int ret;

    ret = api_command_helper(L, "MODE %s %s", target, args);

    lua_pushnumber(L, ret);
    return 1;
}


int
api_set_topic(lua_State *L)
{
    const char *channel  = luaL_checkstring(L, 1);
    const char *newtopic = luaL_checkstring(L, 2);
    int ret;

    ret = api_command_helper(L, "TOPIC %s :%s", channel, newtopic);

    lua_pushnumber(L, ret);
    return 1;
}


int
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


int
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


int
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
