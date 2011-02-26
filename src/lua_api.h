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
 *  Lua script management (lua_api.h)
 *  ---
 *  Load, unload and emit signals to Lua scripts
 *
 *  Created: 25.02.2011 20:24:58
 *
 ******************************************************************************/
#ifndef LUA_API_H
#define LUA_API_H

#include <lua.h>

#include "state.h"
#include "irc.h"
#include "channel.h"
#include "linked_list.h"

#define LIBNAME "luna"


typedef struct luna_script
{
    char filename[256];

    char name[32];
    char description[128];
    char version[16];
    char author[64];

    lua_State *state;
} luna_script;


/* For list_destroy() and list_delete() */
int script_cmp(void *, void *);
void script_free(void *);

int script_load(luna_state *, const char *);
int script_unload(luna_state *, const char *);

int api_push_sender(lua_State *, irc_sender *);
int signal_dispatch(luna_state *, const char *, const char *, ...);
int script_emit(luna_state *, luna_script *, const char *,
                const char *, va_list);
int script_identify(lua_State *, luna_script *);

int api_push_script(lua_State *, luna_script *);
int api_push_channel(lua_State *, irc_channel *);
int api_push_user(lua_State *, irc_user *);

#endif

