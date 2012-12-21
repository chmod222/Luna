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
 *  Lua script management (lua_manager.c)
 *  ---
 *  Manage script loading, unloading and bookkeeping
 *
 *  Created: 03.02.2012 02:25:34
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../mm.h"

#include "lua_manager.h"
#include "lua_util.h"

#include "lua_api_functions.h"
#include "lua_user.h"
#include "lua_script.h"
#include "lua_self.h"
#include "lua_channel.h"
#include "lua_source.h"


int script_emit(luna_state *, luna_script *, const char *, va_list);
int script_identify(lua_State *, luna_script *);

const char *env_key = "LUNA_ENV";


int
script_cmp(const void *data, const void *list_data)
{
    char *name = (char *)data;
    luna_script *script = (luna_script *)list_data;

    return strcmp(name, script->filename);
}


void
script_free(void *list_data)
{
    luna_script *script = (luna_script *)list_data;

    lua_close(script->state);
    mm_free(list_data);

    return;
}


void
script_emit_helper(luna_state *state, luna_script *s,
		   const char *sig, ...)
{
    va_list args;
    va_start(args, sig);
    script_emit(state, s, sig, args);
    va_end(args);
}


int
script_unload(luna_state *state, const char *file)
{
    void *key = (void *)file;
    void *result = list_find(state->scripts, key, &script_cmp);

    if (result)
    {
        /* Call unload signal */
        script_emit_helper(state, (luna_script *)result, "unload", NULL);
        list_delete(state->scripts, result, &script_free);

        return 0;
    }

    return 1;
}


int
script_load(luna_state *state, const char *file)
{
    lua_State *L = NULL;
    luna_script *script = NULL;
    int api_table = 0;

    /* Script can be openened and allocated, or Lua state cannot be created ? */
    if (!(script = mm_malloc(sizeof(*script))) || !((L = lua_newstate(&mm_lalloc, NULL))))
    {
        mm_free(script);

        return 1;
    }

    memset(script, 0, sizeof(*script));
    script->state = L;

    luaL_openlibs(L);

    /* Push Luna state pointer to script so it has access to data
     * _G["LUNA_ENV"] = state */
    lua_pushlightuserdata(L, (void *)env_key);
    lua_pushlightuserdata(L, state);
    lua_settable(L, LUA_REGISTRYINDEX);

    /* Register library methods */
    /* luaL_Register(L, LIBNAME, api_library); */
    api_table = (api_register(L), lua_gettop(L));

    /* Copy the value for call to lua_setglobal() and make the call */
    lua_pushvalue(L, -1);
    lua_setglobal(L, LIBNAME);

    lua_pushstring(L, "types");
    lua_newtable(L);
    lua_settable(L, api_table);

    /* Register custom types */
    luaX_register_user(L, api_table);
    luaX_register_script(L, api_table);
    luaX_register_self(L, api_table);
    luaX_register_channel(L, api_table);
    luaX_register_source(L, api_table);

    /* Register empty callbacks table
     * luna.__callbacks = {} */
    lua_getglobal(L, LIBNAME);
    lua_pushstring(L, "__callbacks");
    lua_newtable(L);
    lua_settable(L, -3);

    /* TODO: Make this... better */
    if (luaL_dofile(L, "corelib.lua") != 0)
    {
        logger_log(state->logger, LOGLEV_WARNING,
                   "Unable to load Luna core library '%s'", "corelib.lua");
        logger_log(state->logger, LOGLEV_WARNING,
                   "Error was: %s", lua_tostring(L, -1));
    }

    /* Clean the stack */
    lua_pop(L, -1);

    /* Execute */
    if ((luaL_dofile(L, file) == 0) && (script_identify(L, script) == 0))
    {
        list_push_back(state->scripts, script);
        strncpy(script->filename, file, sizeof(script->filename) - 1);

        script_emit_helper(state, script, "load", NULL);

        return 0;
    }
    else
    {
        /* luaL_dofile() failure or invalid script */
        logger_log(state->logger, LOGLEV_ERROR, "Lua error: %s",
                   lua_tostring(L, -1));

        script_free(script);
    }

    return 1;
}


int
script_identify(lua_State *L, luna_script *script)
{
    int api_table;
    int info_table;

    /* Strings */
    const char *name = NULL;
    const char *descr = NULL;
    const char *author = NULL;
    const char *version = NULL;

    /* Get API table */
    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    /* Push key of scriptinfo table and get it's value */
    lua_pushstring(L, "__scriptinfo");
    lua_gettable(L, api_table);
    info_table = lua_gettop(L);

    /* Is it defined and a table? */
    if (lua_type(L, info_table) != LUA_TTABLE)
    {
        lua_pop(L, -1);
        return 1;
    }

    lua_pushstring(L, "name");
    lua_gettable(L, info_table);
    name = lua_tostring(L, lua_gettop(L));

    lua_pushstring(L, "description");
    lua_gettable(L, info_table);
    descr = lua_tostring(L, lua_gettop(L));

    lua_pushstring(L, "version");
    lua_gettable(L, info_table);
    version = lua_tostring(L, lua_gettop(L));

    lua_pushstring(L, "author");
    lua_gettable(L, info_table);
    author = lua_tostring(L, lua_gettop(L));

    /* Values must be strings and nonempty */
    if ((name && strcmp(name, ""))   &&
            (descr && strcmp(descr, "")) &&
            (author && strcmp(author, "")) &&
            (version && strcmp(version, "")))
    {
        /* Copy values over */
        strncpy(script->name, name, sizeof(script->name) - 1);
        strncpy(script->description, descr, sizeof(script->description) - 1);
        strncpy(script->version, version, sizeof(script->version) - 1);
        strncpy(script->author, author, sizeof(script->author) - 1);

        lua_pop(L, -1);
        return 0;
    }
    else
    {
        lua_pop(L, -1);
        return 1;
    }
}


int
signal_dispatch(luna_state *state, const char *sig, ...)
{
    va_list args;
    list_node *cur;

    for (cur = state->scripts->root; cur != NULL; cur = cur->next)
    {
        va_start(args, sig);
        script_emit(state, cur->data, sig, args);
        va_end(args);
    }

    return 0;
}


int
script_emit(luna_state *state, luna_script *script, const char *sig,
            va_list vargs)
{
    int api_table;
    int callbacks_array;
    int len;
    int i = 1;

    lua_State *L = script->state;

    /* For each {signal, callback_function} in luna.__callbacks ... */
    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    lua_pushstring(L, "__callbacks");
    lua_gettable(L, api_table);
    callbacks_array = lua_gettop(L);
    len = lua_rawlen(L, callbacks_array);

    while (i <= len)
    {
        int callback_table;
        int j = 0;

        lua_rawgeti(L, callbacks_array, i);
        callback_table = lua_gettop(L);

        lua_pushstring(L, "signal");
        lua_gettable(L, callback_table);

        /* luna.__callbacks[i].signal == sig? */
        if (!strcmp(lua_tostring(L, -1), sig))
        {
            /* Emit */
            va_list args;
            va_copy(args, vargs);

            /* Push function to stack
             * luna.__callbacks[i].callback(...) */
            lua_pushstring(L, "callback");
            lua_gettable(L, callback_table);

            for (j = 0;; ++j)
            {

                luaX_serializable *v = va_arg(args, luaX_serializable *);

                if (v == NULL)
                    break;

                if (v->serialize != NULL)
                {
                    v->serialize(L, v);
                }
                else
                {
                    logger_log(state->logger, LOGLEV_ERROR,
                               "No serializer function for argument %d of "
                               "signal '%s'!", j, sig);

                    lua_pop(L, -1);
                    return 0;
                }
            }

            if (lua_pcall(L, j, 0, 0) != 0)
            {
                logger_log(state->logger, LOGLEV_ERROR,
                           "Lua error ('%s@%s'): %s",
                           sig, script->filename, lua_tostring(L, -1));

                /* Pop off error message */
                lua_pop(L, 1);
            }

            //lua_pop(L, -1);
        }

        lua_pop(L, 2);
        ++i;
    }

    lua_pop(L, -1);
    return 1;
}

