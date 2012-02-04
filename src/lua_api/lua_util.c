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
api_loglevel_from_string(const char *lev)
{
    if (!lev)
        return LOGLEV_ERROR;

    if (!strcasecmp(lev, "info"))
        return LOGLEV_INFO;
    else if (!strcasecmp(lev, "warning"))
        return LOGLEV_WARNING;
    else
        return LOGLEV_ERROR;
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


int
api_setfield_s(lua_State *L, int table, const char *key, const char *value)
{
    lua_pushstring(L, key);
    lua_pushstring(L, value);
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

