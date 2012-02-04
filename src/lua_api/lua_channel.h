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
 *  Lua channel object management (lua_self.h)
 *  ---
 *  Provide access to the global channel list within scripts
 *
 *  Created: 03.02.2012 20:41:01
 *
 ******************************************************************************/
#ifndef LUA_CHANNEL_H
#define LUA_CHANNEL_H

#include <lua.h>


/* Only store enough information to retrieve the actual information
 * via the global state */
typedef struct luaX_channel
{
    char name[64];
} luaX_channel;

typedef struct luaX_chanuser
{
    luaX_channel channel;
    char nick[32];
} luaX_chanuser;


int luaX_register_channel(lua_State*, int);

void luaX_push_chanuser(lua_State*, luaX_chanuser*);
void luaX_push_channel(lua_State*, luaX_channel*);

#endif
