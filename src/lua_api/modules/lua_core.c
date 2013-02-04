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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../lua_util.h"


int luaX_core_log(lua_State *);
int luaX_core_sendline(lua_State *);

const luaL_Reg luaX_core_functions[] =
{
    { "log",             luaX_core_log },
    { "sendline",        luaX_core_sendline },

    { NULL, NULL }
};


int luaX_core_log(lua_State *L)
{
    int level = api_loglevel_from_string(luaL_checkstring(L, 1));
    const char *message = luaL_checkstring(L, 2);
    luna_state *state = api_getstate(L);

    logger_log(state->logger, level, "%s", message);

    return 0;
}

int luaX_core_sendline(lua_State *L)
{
    const char *line = luaL_checkstring(L, 1);
    luna_state *state = api_getstate(L);

    lua_pushnumber(L, net_sendfln(state, "%s", line));

    return 1;
}

int luaX_register_core(lua_State *L)
{
#if LUA_VERSION_NUM == 502
    luaL_newlib(L, luaX_core_functions);
#else
    lua_newtable(L);
    luaL_register(L, NULL, luaX_core_functions);
#endif

    return 0;
}
