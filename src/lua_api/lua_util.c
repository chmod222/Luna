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
 *  Lua API utility functions (lua_util.c)
 *  ---
 *  Provide pushing and checking functions for internal structures
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
#include "lua_util.h"
#include "../user.h"
#include "../channel.h"

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

    api_setfield_n(L, table, "created", channel->created);

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

    lua_pushstring(L, "op");
    lua_pushboolean(L, user->op);
    lua_settable(L, table);

    lua_pushstring(L, "voice");
    lua_pushboolean(L, user->voice);
    lua_settable(L, table);

    return 0;
}


int
api_push_luna_user(lua_State *L, luna_user *user)
{
    int table;

    lua_newtable(L);
    table = lua_gettop(L);

    api_setfield_s(L, table, "id", user->id);
    api_setfield_s(L, table, "hostmask", user->hostmask);
    api_setfield_s(L, table, "flags", user->flags);
    api_setfield_s(L, table, "level", user->level);

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

luna_user *
api_checkuser(lua_State *L, int index)
{
    int h;

    luna_state *state = api_getstate(L);
    const char *m = NULL;

    if (lua_type(L, 1) != LUA_TTABLE)
        return NULL;

    lua_pushstring(L, "hostmask");
    lua_gettable(L, index);
    h = lua_gettop(L);

    /* checking hostmask should do */
    m = lua_tostring(L, h);

    return (luna_user *)list_find(state->users, (void *)m, &luna_user_host_cmp);
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

    lua_close(script->state);
    free(list_data);

    return;
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

