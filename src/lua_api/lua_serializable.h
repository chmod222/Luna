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
#ifndef LUA_SERIALIZABLE_H
#define LUA_SERIALIZABLE_H

#include <lua.h>

#include "../irc.h"

/*
 * Implemented by:
 *  * luaX_string    (lua_serializable.h)
 *  * luaX_channel   (lua_channel.h)
 *  * luaX_chanuser  (lua_channel.h)
 *  * irc_sender     (irc.h)
 */
typedef struct luaX_serializable
{
    /*
     * Basically an interface like you would expect in OOP languages such as
     * Java
     *
     * Every luaX_[type] declares this same function pointer as the first
     * field so pointers to these structures can be typed as luaX_serializable
     * and reference their own serialize functions. Implemented by all types
     * passed into signal_emit().
     */
    int (*serialize)(lua_State *, struct luaX_serializable *);
} luaX_serializable;

typedef struct luaX_string
{
    int (*serialize)(lua_State *, struct luaX_serializable *);

    const char *string;
} luaX_string;

typedef struct luaX_string_array
{
    int (*serialize)(lua_State *, struct luaX_serializable *);

    const char **array;
    size_t len;
} luaX_string_array;

int luaX_push_string(lua_State *, luaX_serializable *);
int luaX_push_event(lua_State *, luaX_serializable *);

luaX_string luaX_make_string(const char *);
luaX_string_array luaX_make_string_array(const char **, size_t);

#endif
