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
 *  Lua channel object management (lua_channel.h)
 *  ---
 *  Provide meta info about active channels to scripts
 *
 *  Created: 03.02.2012 19:37:01
 *
 ******************************************************************************/
#ifndef LUA_CHANNEL
#define LUA_CHANNEL

#include <lua.h>

int luaX_register_channel(lua_State *, int);

#endif
