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
 *  TODO: * Prevent the script from loading or unloading itself
 *          (or fix the segfault)
 *
 *  Created: 03.02.2012 17:57:01
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "lua_util.h"
#include "lua_manager.h"
#include "lua_script.h"


int luaX_script_getscriptinfo(lua_State*);
int luaX_script_isself(lua_State*);

int luaX_scripts_load(lua_State*);
int luaX_scripts_unload(lua_State*);
int luaX_scripts_getall(lua_State*);

static const struct luaL_Reg luaX_script_functions[] = {
    { "load", luaX_scripts_load },
    { "unload", luaX_scripts_unload },
    { "getScriptList", luaX_scripts_getall },

    { NULL, NULL }
};


static const struct luaL_Reg luaX_script_methods[] = {
    { "getScriptInfo", luaX_script_getscriptinfo },
    { "isSelf", luaX_script_isself },

    { NULL, NULL }
};


int
luaX_script_getscriptinfo(lua_State *L)
{
    luna_script *ud = (luna_script *)luaL_checkudata(L, 1, "luna.script");

    lua_pushstring(L, ud->filename);
    lua_pushstring(L, ud->name);
    lua_pushstring(L, ud->description);
    lua_pushstring(L, ud->author);
    lua_pushstring(L, ud->version);

    return 5;
}


int
luaX_script_isself(lua_State *L)
{
    luna_script *ud = (luna_script *)luaL_checkudata(L, 1, "luna.script");

    lua_pushboolean(L, ud->state == L);
    return 1;
}


int
luaX_scripts_load(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    luna_state *state = api_getstate(L);

    if (!script_load(state, file))
    {
        luna_script *s = (luna_script *)list_find(
                state->scripts, (void *)file, &script_cmp);
        luna_script *u = (luna_script *)lua_newuserdata(L, sizeof(luna_script));

        luaL_getmetatable(L, "luna.script");
        lua_setmetatable(L, -2);

        memcpy(u, s, sizeof(luna_script));

        return 1;
    }
    else
    {
        return 0;
    }
}


int
luaX_scripts_unload(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    luna_state *state = api_getstate(L);

    luna_script *script = (luna_script *)list_find(
            state->scripts, (void *)file, &script_cmp);

    if (script)
    {
        if (script->state == L)
            return luaL_error(L, "cannot unload self");
        else
            script_unload(state, file);
    }

    return 0;
}


int
luaX_scripts_getall(lua_State *L)
{
    int array;
    int i = 1;
    luna_state *state;
    list_node *cur = NULL;

    state = api_getstate(L);

    lua_newtable(L);
    array = lua_gettop(L);

    for (cur = state->scripts->root; cur != NULL; cur = cur->next)
    {
        luna_script *u = (luna_script *)lua_newuserdata(L, sizeof(luna_script));
        luaL_getmetatable(L, "luna.script");
        lua_setmetatable(L, -2);

        memcpy(u, cur->data, sizeof(luna_script));

        lua_rawseti(L, array, i++);
    }

    return 1;
}


int
luaX_register_script(lua_State *L, int regtable)
{
    int meta = (luaL_newmetatable(L, "luna.script"), lua_gettop(L));

    /* Register indexing field */
    lua_pushstring(L, "__index");
    lua_pushvalue(L, meta);
    lua_settable(L, meta);

    /* Register all methods within this table */
    luaL_openlib(L, NULL, luaX_script_methods, 0);

    /* And register other functions inside regtable
     * luna.script = { ... } */
    lua_pushstring(L, "scripts");
    /*
     * lua_newtable(L);
     * luaL_Register(L, NULL, luaX_script_functions);
     */
    luaL_newlib(L, luaX_script_functions);
    lua_settable(L, regtable);

    /* Finally, register the meta table for extending */
    lua_pushstring(L, "types");
    lua_gettable(L, regtable);

    lua_pushstring(L, "script");
    lua_pushvalue(L, meta);

    lua_settable(L, -3);


    return 1;
}

int
luaX_push_script(lua_State *L, luaX_serializable *_scr)
{
    const luna_script *script = &(((luaX_script*)_scr)->script);

    luna_script *u = (luna_script *)lua_newuserdata(L, sizeof(luna_script));
    luaL_getmetatable(L, "luna.script");
    lua_setmetatable(L, -2);

    memcpy(u, script, sizeof(luna_script));

    return 0;
}

luaX_script
luaX_make_script(const luna_script *script)
{
    luaX_script ret = { &luaX_push_script, *script };

    return ret;
}


