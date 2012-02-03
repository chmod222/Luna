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
 *  Lua api functions (lua_api_functions.h)
 *  ---
 *  Functions to register in each script environment
 *
 *  Created: 03.02.2012 02:25:34
 *
 ******************************************************************************/
#ifndef LUA_API_FUNCTIONS_H
#define LUA_API_FUNCTIONS_H

#include <lua.h>

extern luaL_Reg api_library[];

int api_script_register(lua_State *);

int api_get_user(lua_State *);
int api_add_user(lua_State *);
int api_remove_user(lua_State *);
int api_reload_userlist(lua_State *);

int api_set_user_flags(lua_State *);
int api_set_user_level(lua_State *);
int api_set_user_id(lua_State *);

int api_user_flag_set(lua_State *);

int api_load_script(lua_State *);
int api_unload_script(lua_State *);
int api_log(lua_State *);

int api_sendline(lua_State *);

int api_channels(lua_State *);
int api_scripts(lua_State *);
int api_info(lua_State *);
int api_users(lua_State *);

#endif
