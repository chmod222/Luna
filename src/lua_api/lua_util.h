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
 *  Lua API utility functions (lua_util.h)
 *  ---
 *  Provide pushing and checking functions for internal structures
 *
 *  Created: 03.02.2012 02:25:34
 *
 ******************************************************************************/
#ifndef LUA_UTIL
#define LUA_UTIL

#include <lua.h>

#include "lua_manager.h"
#include "../channel.h"


int api_loglevel_from_string(const char *);
luna_state *api_getstate(lua_State *);

int luaX_push_raw(luna_state *, lua_State *, va_list);
int luaX_push_idle(luna_state *, lua_State *, va_list);
int luaX_push_privmsg(luna_state *, lua_State *, va_list);
int luaX_push_ctcp(luna_state *, lua_State *, va_list);
int luaX_push_ctcp_rsp(luna_state *, lua_State *, va_list);
int luaX_push_action(luna_state *, lua_State *, va_list);
int luaX_push_command(luna_state *, lua_State *, va_list);
int luaX_push_join(luna_state *, lua_State *, va_list);
int luaX_push_join_sync(luna_state *, lua_State *, va_list);
int luaX_push_part(luna_state *, lua_State *, va_list);
int luaX_push_quit(luna_state *, lua_State *, va_list);
int luaX_push_notice(luna_state *, lua_State *, va_list);
int luaX_push_nick(luna_state *, lua_State *, va_list);
int luaX_push_invite(luna_state *, lua_State *, va_list);
int luaX_push_topic(luna_state *, lua_State *, va_list);
int luaX_push_kick(luna_state *, lua_State *, va_list);
int luaX_push_script_load(luna_state *, lua_State *, va_list);
int luaX_push_script_unload(luna_state *, lua_State *, va_list);

#endif
