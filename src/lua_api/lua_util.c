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


luna_state *
api_getstate(lua_State *L)
{
    luna_state *state = NULL;

    lua_pushlightuserdata(L, (void *)env_key);
    lua_gettable(L, LUA_REGISTRYINDEX);

    state = lua_touserdata(L, -1);
    lua_pop(L, 1);

    return state;
}

