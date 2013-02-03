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
 *  Lua script object management (lua_self.h)
 *  ---
 *  Provide meta info about loaded scripts to scripts
 *
 *  Created: 03.02.2012 19:37:01
 *
 ******************************************************************************/
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../lua_util.h"

int luaX_script_getloadedscripts(lua_State *);
int luaX_script_getscriptinfo(lua_State *);
int luaX_script_load(lua_State *);
int luaX_script_unload(lua_State *);
int luaX_script_getself(lua_State *);


static const struct luaL_Reg luaX_script_functions[] =
{
    { "get_loaded_scripts", luaX_script_getloadedscripts },
    { "get_script_info", luaX_script_getscriptinfo },
    { "load_script", luaX_script_load },
    { "unload_script", luaX_script_unload },
    { "get_self", luaX_script_getself },

    { NULL, NULL }
};


int
luaX_push_scriptinfo(lua_State *L, luna_script *script)
{
    int table = (lua_newtable(L), lua_gettop(L));

    lua_pushstring(L, "filename");
    lua_pushstring(L, script->filename);
    lua_settable(L, table);

    lua_pushstring(L, "name");
    lua_pushstring(L, script->name);
    lua_settable(L, table);

    lua_pushstring(L, "description");
    lua_pushstring(L, script->description);
    lua_settable(L, table);

    lua_pushstring(L, "author");
    lua_pushstring(L, script->author);
    lua_settable(L, table);

    lua_pushstring(L, "version");
    lua_pushstring(L, script->version);
    lua_settable(L, table);

    return 1;
}


int
luaX_script_getloadedscripts(lua_State *L)
{
    int arr;
    int i = 1;
    list_node *cur;

    luna_state *state = api_getstate(L);

    arr = (lua_newtable(L), lua_gettop(L));
    for (cur = state->scripts->root; cur != NULL; cur = cur->next)
    {
        lua_pushstring(L, ((luna_script *)(cur->data))->filename);
        lua_rawseti(L, arr, i++);
    }

    return 1;
}



int
luaX_script_getscriptinfo(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    void *result;

    luna_state *state = api_getstate(L);

    result = list_find(state->scripts, file, &script_cmp);

    if (result)
    {
        luna_script *script = (luna_script *)result;

        luaX_push_scriptinfo(L, script);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}


int
luaX_script_load(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    luna_script *result;

    luna_state *state = api_getstate(L);

    result = list_find(state->scripts, file, &script_cmp);

    if (result)
    {
        return luaL_error(L, "script '%s' already loaded", file);
    }
    else
    {
        if (!script_load(state, file))
        {
            result = list_find(state->scripts, file, &script_cmp);

            return luaX_push_scriptinfo(L, result);
        }
        else
        {
            return luaL_error(L, "failed to load script '%s'", file);
        }
    }
}


int
luaX_script_unload(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    luna_script *result;

    luna_state *state = api_getstate(L);

    result = list_find(state->scripts, file, &script_cmp);

    if (!result)
    {
        return luaL_error(L, "script '%s' not loaded", file);
    }
    else
    {
        luna_script script = *result;

        if (script.state == L)
            return luaL_error(L, "cannot unload self");

        if (!script_unload(state, file))
            return luaX_push_scriptinfo(L, &script);
        else
            return luaL_error(L, "failed to load script '%s'", file);
    }
    return 0;
}


int
luaX_script_getself(lua_State *L)
{
    list_node *cur;
    luna_state *state = api_getstate(L);

    for (cur = state->scripts->root; cur != NULL; cur = cur->next)
    {
        if (((luna_script *)(cur->data))->state == L)
        {
            lua_pushstring(L, ((luna_script *)(cur->data))->filename);
            return 1;
        }
    }

    /* Not gonna happen */
    return 0;
}

int
luaX_register_script(lua_State *L, int regtable)
{
    /* Register functions inside regtable
     * luna.scripts = { ... } */
    lua_pushstring(L, "scripts");

#if LUA_VERSION_NUMBER
    luaL_newlib(L, luaX_script_functions);
#else
    lua_newtable(L);
    luaL_register(L, NULL, luaX_script_functions);
#endif

    lua_settable(L, regtable);

    return 1;
}
