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

    { "load_script",     api_load_script },
    { "unload_script",   api_unload_script },

    { "log",             api_log },

    { "sendline",        api_sendline },

    { "channels",        api_channels },
    { "scripts",         api_scripts },
    { "info",            api_info },

    { NULL, NULL }
};


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



