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
 *  Lua script object management (lua_script.h)
 *  ---
 *  Provide access to Luna scripts within scripts
 *
 *  Created: 03.02.2012 17:57:01
 *
 ******************************************************************************/
#ifndef LUA_SCRIPT_H
#define LUA_SCRIPT_H

#include <lua.h>

/* No need to create middle-man structures for script objects, since they
 * are immutable and need no constant access to the global state */

int luaX_register_script(lua_State*, int);

int luaX_make_script(luna_script*);
int luaX_push_script(lua_State*, luaX_serializable*);

#endif
