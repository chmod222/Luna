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
 *  Lua user object management (lua_user.h)
 *  ---
 *  Provide access to Luna users within scripts
 *
 *  Created: 03.02.2012 16:39:01
 *
 ******************************************************************************/
#ifndef LUA_USER_H
#define LUA_USER_H

#include <lua.h>


typedef struct luaX_user
{
    char hostmask[128];
} luaX_user;


int luaX_register_user(lua_State*, int);

#endif
