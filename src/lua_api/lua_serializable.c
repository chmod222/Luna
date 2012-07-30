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
 *  Serializable objects for Lua (lua_serializable.c)
 *  ---
 *  Provide access to the global channel list within scripts
 *
 *  Created: 29.07.2012 23:56:12
 *
 ******************************************************************************/
#include <lua.h>

#include <string.h>

#include "lua_channel.h"
#include "lua_serializable.h"

int luaX_push_string(lua_State *L, luaX_serializable *_s)
{
    luaX_string *s = (luaX_string*)_s;
    lua_pushstring(L, s->string);

    return 0;
}

luaX_string luaX_make_string(const char *str)
{
    luaX_string ret = { &luaX_push_string, str };
    return ret;
}
