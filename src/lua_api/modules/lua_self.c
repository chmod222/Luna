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
 *  Lua state object management (lua_self.h)
 *  ---
 *  Provide access to the global Luna state within scripts
 *
 *  Created: 03.02.2012 19:37:01
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../../net.h"
#include "../../state.h"
#include "../../mm.h"
#include "../lua_util.h"

int luaX_self_getuserinfo(lua_State *);
int luaX_self_getserver(lua_State *);
int luaX_self_getmeminfo(lua_State *);
int luaX_self_getruntimes(lua_State *);


static const struct luaL_Reg luaX_self_functions[] =
{
    { "get_user_info", luaX_self_getuserinfo },
    { "get_server_info", luaX_self_getserver },
    { "get_memory_info", luaX_self_getmeminfo },
    { "get_runtime_info", luaX_self_getruntimes },

    { NULL, NULL }
};


int
luaX_self_getuserinfo(lua_State *L)
{
    int table;

    luna_state *state = api_getstate(L);

    table = (lua_newtable(L), lua_gettop(L));

    lua_pushstring(L, "nick");
    lua_pushstring(L, state->userinfo.nick);
    lua_settable(L, table);

    lua_pushstring(L, "user");
    lua_pushstring(L, state->userinfo.user);
    lua_settable(L, table);

    lua_pushstring(L, "realname");
    lua_pushstring(L, state->userinfo.real);
    lua_settable(L, table);

    return 1;
}


int
luaX_self_getserver(lua_State *L)
{
    luna_state *state = api_getstate(L);

    lua_pushstring(L, state->serverinfo.host);
    lua_pushnumber(L, state->serverinfo.port);

    return 2;
}


int
luaX_self_getmeminfo(lua_State *L)
{
    int table = (lua_newtable(L), lua_gettop(L));

    lua_pushstring(L, "used");
    lua_pushnumber(L, mm_inuse());
    lua_settable(L, table);

    lua_pushstring(L, "allocs");
    lua_pushnumber(L, mm_state.mm_allocs);
    lua_settable(L, table);

    lua_pushstring(L, "frees");
    lua_pushnumber(L, mm_state.mm_frees);
    lua_settable(L, table);

    return 1;
}


int
luaX_self_getruntimes(lua_State *L)
{
    int table;

    luna_state *state = api_getstate(L);

    table = (lua_newtable(L), lua_gettop(L));

    lua_pushstring(L, "started");
    lua_pushnumber(L, state->started);
    lua_settable(L, table);

    lua_pushstring(L, "connected");
    lua_pushnumber(L, state->connected);
    lua_settable(L, table);

    return 1;
}


int
luaX_register_self(lua_State *L, int regtable)
{
    /* Register functions inside regtable
     * luna.self = { ... } */
    lua_pushstring(L, "self");

#if LUA_VERSION_NUM == 502
    luaL_newlib(L, luaX_self_functions);
#else
    lua_newtable(L);
    luaL_register(L, NULL, luaX_self_functions);
#endif

    lua_settable(L, regtable);

    return 1;
}
