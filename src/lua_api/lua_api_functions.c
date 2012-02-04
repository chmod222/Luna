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
    { "log",             api_log },
    { "sendLine",        api_sendline },

    { NULL, NULL }
};


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
