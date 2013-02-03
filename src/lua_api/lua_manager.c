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

#include "lua_util.h"

#include "modules/lua_core.h"
#include "modules/lua_self.h"
#include "modules/lua_script.h"
#include "modules/lua_channel.h"
#include "modules/lua_user.h"

int script_emit(luna_state *, luna_script *, const char *,
                luaX_push_helper, va_list vargs);
int script_identify(luna_state *, lua_State *, luna_script *);

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


int
script_identify(luna_state *state, lua_State *L, luna_script *script)
{
    int api_table;

    /* Get API table */
    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    lua_pushstring(L, "register_script");
    lua_gettable(L, api_table);

    if (lua_pcall(L, 0, 4, 0) == 0)
    {
        /* Pop off error message */
        const char *name = lua_tostring(L, -4);
        const char *descr = lua_tostring(L, -3);
        const char *author = lua_tostring(L, -2);
        const char *version = lua_tostring(L, -1);

        if (name && descr && author && version)
        {
            strncpy(script->name, name, sizeof(script->name));
            strncpy(script->description, descr, sizeof(script->description));
            strncpy(script->version, version, sizeof(script->version));
            strncpy(script->author, author, sizeof(script->author));

            return 0;
        }

        lua_pushstring(L, "invalid script registration");

        return 1;
    }

    logger_log(state->logger, LOGLEV_ERROR,
               "Lua error ('luna.register_script@%s'): %s",
               script->filename, lua_tostring(L, -1));

    return 1;
}


int
script_unload(luna_state *state, const char *file)
{
    void *key = (void *)file;
    void *result = list_find(state->scripts, key, &script_cmp);

    if (result)
    {
        /* Call unload signal */
        signal_dispatch(state, "script_unload", &luaX_push_script_unload,
                        file, NULL);

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
    //if (!(script = mm_malloc(sizeof(*script))) || !((L = lua_newstate(&mm_lalloc, NULL))))

    if (!(script = mm_malloc(sizeof(*script))) || !((L = luaL_newstate())))
    {
        mm_free(script);

        return 1;
    }

    memset(script, 0, sizeof(*script));
    script->state = L;
    strncpy(script->filename, file, sizeof(script->filename));

    luaL_openlibs(L);

    /* Push Luna state pointer to script so it has access to data */
    lua_pushlightuserdata(L, (void *)env_key);
    lua_pushlightuserdata(L, state);
    lua_settable(L, LUA_REGISTRYINDEX);

    /* Register core library methods */
    api_table = (luaX_register_core(L), lua_gettop(L));

    /* Copy the value for call to lua_setglobal() and make the call */
    lua_pushvalue(L, -1);
    lua_setglobal(L, LIBNAME);

    /* Register modules */
    luaX_register_self(L, api_table);   /* luna.self */
    luaX_register_script(L, api_table); /* luna.scripts */
    luaX_register_channel(L, api_table); /* luna.channels */
    luaX_register_user(L, api_table); /* luna.users */

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
    if ((luaL_dofile(L, file) == 0) && (script_identify(state, L, script) == 0))
    {
        list_push_back(state->scripts, script);
        strncpy(script->filename, file, sizeof(script->filename) - 1);

        signal_dispatch(state, "script_load", &luaX_push_script_load,
                        file, NULL);

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
signal_dispatch(luna_state *state, const char *sig, luaX_push_helper f, ...)
{
    va_list args;
    list_node *cur;

    for (cur = state->scripts->root; cur != NULL; cur = cur->next)
    {
        va_start(args, f);
        script_emit(state, cur->data, sig, f, args);
        va_end(args);
    }

    return 0;
}


int
script_emit(luna_state *state, luna_script *script, const char *sig,
            luaX_push_helper f, va_list vargs)
{
    int api_table;
    int i = 1;

    lua_State *L = script->state;

    lua_getglobal(L, LIBNAME);
    api_table = lua_gettop(L);

    lua_pushstring(L, "emit_signal");
    lua_gettable(L, api_table);

    lua_pushstring(L, sig);

    /* Push arguments */
    if (f != NULL)
        i += f(state, script->state, vargs);

    if (lua_pcall(L, i, 0, 0) != 0)
    {
        logger_log(state->logger, LOGLEV_ERROR,
                   "Lua error ('%s@%s'): %s",
                   sig, script->filename, lua_tostring(L, -1));

        /* Pop off error message */
        lua_pop(L, 1);
    }

    lua_pop(L, -1);
    return 1;
}

