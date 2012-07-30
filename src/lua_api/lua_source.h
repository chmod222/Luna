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
 *  Lua source object management (lua_source.h)
 *  ---
 *  Provide access to message sources (user, server(?)) within scripts
 *
 *  Created: 08.02.2012 05:21:03
 *
 ******************************************************************************/
#ifndef LUA_SOURCE_H
#define LUA_SOURCE_H

#include <lua.h>

#include "lua_serializable.h"


int luaX_register_source(lua_State*, int);

int luaX_push_irc_sender(lua_State*, luaX_serializable*);

int luaX_make_irc_sender(irc_sender*);

#endif
