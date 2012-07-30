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
 *  Lua source object management (lua_source.c)
 *  ---
 *  Provide access to message sources (user, server(?)) within scripts
 *
 *  Created: 08.02.2012 05:21:03
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../irc.h"
#include "lua_serializable.h"
#include "lua_source.h"


int luaX_user_getuserinfo(lua_State *L);

static const struct luaL_reg luaX_user_methods[] = {
    { "getUserInfo", luaX_user_getuserinfo },
    { NULL, NULL }
};


int
luaX_push_source(lua_State *L, luaX_serializable *_s)
{
    luaX_source *s = (luaX_source*)_s;
    irc_sender *u = (irc_sender *)lua_newuserdata(L, sizeof(irc_sender));

    luaL_getmetatable(L, "luna.source.user");
    lua_setmetatable(L, -2);

    memcpy(u, &(s->source), sizeof(irc_sender));

    return 0;
}


int
luaX_user_getuserinfo(lua_State *L)
{
    irc_sender *s = (irc_sender *)luaL_checkudata(L, 1, "luna.source.user");

    lua_pushstring(L, s->nick);
    lua_pushstring(L, s->user);
    lua_pushstring(L, s->host);

    return 3;
}


int
luaX_register_source(lua_State *L, int regtable)
{
    int meta = (luaL_newmetatable(L, "luna.source.user"), lua_gettop(L));

    lua_pushstring(L, "__index");
    lua_pushvalue(L, meta);
    lua_settable(L, meta);

    luaL_openlib(L, NULL, luaX_user_methods, 0);

    lua_pushstring(L, "types");
    lua_gettable(L, regtable);

    lua_pushstring(L, "source_user");
    lua_pushvalue(L, meta);
    lua_settable(L, -3);

    return 1;
}

luaX_source
luaX_make_source(irc_sender *s)
{
    luaX_source ret = { &luaX_push_source, *s };

    return ret;
}

