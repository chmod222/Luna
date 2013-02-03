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
 *  Lua script management (lua_manager.h)
 *  ---
 *  Manage script loading, unloading and bookkeeping
 *
 *  Created: 03.02.2012 02:25:34
 *
 ******************************************************************************/
#ifndef LUA_MANAGER_H
#define LUA_MANAGER_H

#include "../state.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

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

typedef int (*luaX_push_helper)(luna_state *, lua_State *, va_list);

extern const char *env_key;

int script_cmp(const void *, const void *);
int script_load(luna_state *, const char *);
int script_unload(luna_state *, const char *);
void script_free(void *);

int signal_dispatch(luna_state *, const char *, luaX_push_helper, ...);

#endif
