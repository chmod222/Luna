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

#include "../net.h"
#include "../state.h"
#include "lua_util.h"


int luaX_self_getuserinfo(lua_State*);
int luaX_self_getserver(lua_State*);
int luaX_self_getruntimes(lua_State*);


static const struct luaL_reg luaX_self_functions[] = {
    { "get_userinfo", luaX_self_getuserinfo },
    { "get_server", luaX_self_getserver },
    { "get_runtimes", luaX_self_getruntimes },

    { NULL, NULL }
};


int
luaX_self_getuserinfo(lua_State *L)
{
    luna_state *state = api_getstate(L);

    lua_pushstring(L, state->userinfo.nick);
    lua_pushstring(L, state->userinfo.user);
    lua_pushstring(L, state->userinfo.real);

    return 3;
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
luaX_self_getruntimes(lua_State *L)
{
    luna_state *state = api_getstate(L);

    lua_pushnumber(L, state->started);
    lua_pushnumber(L, state->connected);

    return 2;
}


int
luaX_register_self(lua_State *L, int regtable)
{
    /* And register other functions inside regtable
     * luna.self = { ... } */
    lua_pushstring(L, "self");
    lua_newtable(L);
    luaL_register(L, NULL, luaX_self_functions);
    lua_settable(L, regtable);

    return 1;
}
