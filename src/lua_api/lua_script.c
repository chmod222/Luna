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


int luaX_free_script(lua_State*);
int luaX_script_getfilename(lua_State*);
int luaX_script_getname(lua_State*);
int luaX_script_getdescription(lua_State*);
int luaX_script_getversion(lua_State*);
int luaX_script_getauthor(lua_State*);
int luaX_script_isself(lua_State*);


int luaX_scripts_load(lua_State*);
int luaX_scripts_unload(lua_State*);
int luaX_scripts_getall(lua_State*);

static const struct luaL_reg luaX_script_functions[] = {
    { "load", luaX_scripts_load },
    { "unload", luaX_scripts_unload },
    { "get_all", luaX_scripts_getall },

    { NULL, NULL }
};


static const struct luaL_reg luaX_script_methods[] = {
    { "get_filename", luaX_script_getfilename },
    { "get_name", luaX_script_getname },
    { "get_description", luaX_script_getdescription },
    { "get_version", luaX_script_getversion },
    { "get_author", luaX_script_getauthor },
    { "is_self", luaX_script_isself },

    { NULL, NULL }
};


int
luaX_script_getfilename(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.script");
    luaL_argcheck(L, ud != NULL, 1, "'luna.script' expected");

    lua_pushstring(L, ((luna_script *)ud)->filename);
    return 1;
}


int
luaX_script_getname(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.script");
    luaL_argcheck(L, ud != NULL, 1, "'luna.script' expected");

    lua_pushstring(L, ((luna_script *)ud)->name);
    return 1;
}


int
luaX_script_getdescription(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.script");
    luaL_argcheck(L, ud != NULL, 1, "'luna.script' expected");

    lua_pushstring(L, ((luna_script *)ud)->description);
    return 1;
}


int
luaX_script_getversion(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.script");
    luaL_argcheck(L, ud != NULL, 1, "'luna.script' expected");

    lua_pushstring(L, ((luna_script *)ud)->version);
    return 1;
}


int
luaX_script_getauthor(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.script");
    luaL_argcheck(L, ud != NULL, 1, "'luna.script' expected");

    lua_pushstring(L, ((luna_script *)ud)->author);
    return 1;
}


int
luaX_script_isself(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.script");
    luaL_argcheck(L, ud != NULL, 1, "'luna.script' expected");

    lua_pushboolean(L, ((luna_script *)ud)->state == L);
    return 1;
}


int
luaX_scripts_load(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    luna_state *state = api_getstate(L);

    if (!script_load(state, file))
    {
        void *script = list_find(state->scripts, (void *)file, &script_cmp);
        luna_script *s = (luna_script *)script;
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

    script_unload(state, file);

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
luaX_free_script(lua_State *L)
{
    return 0;
}


int
luaX_register_script(lua_State *L, int regtable)
{
    int meta = (luaL_newmetatable(L, "luna.script"), lua_gettop(L));

    /* Register garbage collector function */
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, luaX_free_script);
    lua_settable(L, meta);

    /* Register indexing field */
    lua_pushstring(L, "__index");
    lua_pushvalue(L, meta);
    lua_settable(L, meta);

    /* Register all methods within this table */
    luaL_openlib(L, NULL, luaX_script_methods, 0);

    /* And register other functions inside regtable
     * luna.script = { ... } */
    lua_pushstring(L, "scripts");
    lua_newtable(L);
    luaL_register(L, NULL, luaX_script_functions);
    lua_settable(L, regtable);

    return 1;
}
