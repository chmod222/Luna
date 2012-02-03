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

#include "lua_manager.h"
#include "lua_util.h"

#include "lua_api_functions.h"


int script_emit(luna_state *, luna_script *, const char *,
                const char *, va_list);

int script_identify(lua_State *, luna_script *);

const char *env_key = "LUNA_ENV";


int
script_unload(luna_state *state, const char *file)
{
    void *key = (void *)file;
    void *result = list_find(state->scripts, key, &script_cmp);

    if (result)
    {
        /* Call unload signal */
        script_emit(state, (luna_script *)result, "unload", NULL, NULL);
        list_delete(state->scripts, result, &script_free);

        return 0;
    }

    return 1;
}


int
script_load(luna_state *state, const char *file)
{
    lua_State *L;
    luna_script *script;

    /* Script can be openened and allocated? */
    if ((script = malloc(sizeof(*script))) == NULL)
        return 1;

    if ((L = lua_open()) == NULL)
    {
        free(script);

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
    luaL_register(L, LIBNAME, api_library);

    /* Register empty callbacks table
     * luna.__callbacks = {} */
    lua_getglobal(L, LIBNAME);
    lua_pushstring(L, "__callbacks");
    lua_newtable(L);
    lua_settable(L, -3);

    api_push_loglevels(L);

    /* Clean the stack */
    lua_pop(L, -1);

    /* Execute */
    if ((luaL_dofile(L, file) == 0) && (script_identify(L, script) == 0))
    {
        list_push_back(state->scripts, script);
        strncpy(script->filename, file, sizeof(script->filename) - 1);

        script_emit(state, script, "load", NULL, NULL);

        return 0;
    }
    else
    {
        /* luaL_dofile() failure or invalid script */
        logger_log(state->logger, LOGLEV_ERROR, "Lua error: %s",
                   lua_tostring(L, -1));

        free(script);
        lua_close(L);
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
signal_dispatch(luna_state *state, const char *sig, const char *fmt, ...)
{
    va_list args;
    list_node *cur;

    for (cur = state->scripts->root; cur != NULL; cur = cur->next)
    {
        va_start(args, fmt);
        script_emit(state, cur->data, sig, fmt, args);
        va_end(args);
    }

    return 0;
}


int
script_emit(luna_state *state, luna_script *script, const char *sig,
            const char *fmt, va_list args)
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
    len = lua_objlen(L, callbacks_array);

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

            /* Push function to stack
             * luna.__callbacks[i].callback(...) */
            lua_pushstring(L, "callback");
            lua_gettable(L, callback_table);

            if (fmt != NULL)
            {
                for (j = 0; fmt[j] != 0; ++j)
                {
                    switch (fmt[j])
                    {
                        case 's':
                            lua_pushstring(L, va_arg(args, char *));
                            break;

                        case 'p':
                            api_push_sender(L, va_arg(args, irc_sender *));
                            break;

                        case 'n':
                            lua_pushnumber(L, va_arg(args, double));
                            break;

                        default:
                            j--;
                            break;
                    }
                }
            }

            if (lua_pcall(L, j, 0, 0) != 0)
                logger_log(state->logger, LOGLEV_ERROR,
                           "Lua error ('%s@%s'): %s",
                           sig, script->filename, lua_tostring(L, -1));

            lua_pop(L, -1);
            return 0;
        }

        lua_pop(L, 2);
        ++i;
    }

    lua_pop(L, -1);
    return 1;
}

