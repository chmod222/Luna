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
 *  Lua user object management (lua_user.c)
 *  ---
 *  Provide access to Luna users within scripts
 *
 *  Created: 03.02.2012 16:39:01
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../user.h"
#include "../linked_list.h"

#include "lua_util.h"


int luaX_free_user(lua_State*);
int luaX_user_getflags(lua_State*);
int luaX_user_setflags(lua_State*);
int luaX_user_getlevel(lua_State*);
int luaX_user_setlevel(lua_State*);
int luaX_user_getid(lua_State*);
int luaX_user_setid(lua_State*);

int luaX_users_find(lua_State*);
int luaX_users_add(lua_State*);
int luaX_users_remove(lua_State*);
int luaX_users_save(lua_State*);
int luaX_users_getall(lua_State*);
int luaX_users_reload(lua_State*);

luna_user *find_user_by_userdata(lua_State*, luna_user*);

static const struct luaL_reg luaX_user_functions[] = {
    { "find", luaX_users_find },
    { "add", luaX_users_add },
    { "remove", luaX_users_remove },
    { "save", luaX_users_save },
    { "get_all", luaX_users_getall },
    { "reload", luaX_users_reload },

    { NULL, NULL }
};


static const struct luaL_reg luaX_user_methods[] = {
    { "get_flags", luaX_user_getflags },
    { "set_flags", luaX_user_setflags },

    { "get_level", luaX_user_getlevel },
    { "set_level", luaX_user_setlevel },

    { "get_id", luaX_user_getid },
    { "set_id", luaX_user_setid },

    { NULL, NULL }
};


luna_user *
find_user_by_userdata(lua_State *L, luna_user *u)
{
    luna_state *state = api_getstate(L);

    return (luna_user *)list_find(state->users, (void *)u->hostmask,
                                  &luna_user_host_cmp);
}


int
luaX_user_getflags(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.user");
    luaL_argcheck(L, ud != NULL, 1, "'luna.user' expected");

    lua_pushstring(L, ((luna_user *)ud)->flags);
    return 1;
}


int
luaX_user_setflags(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.user");
    const char *flags = luaL_checkstring(L, 2);
    luna_user *target = NULL;

    luaL_argcheck(L, ud != NULL, 1, "'luna.user' expected");

    if ((target = find_user_by_userdata(L, ud)) != NULL)
    {
        strncpy(target->flags, flags, sizeof(target->flags));
        memcpy(ud, target, sizeof(luna_user));
    }
    else
    {
        return luaL_error(L, "no such user '%s'", ((luna_user *)ud)->hostmask);
    }

    return 0;
}


int
luaX_user_getlevel(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.user");
    luaL_argcheck(L, ud != NULL, 1, "'luna.user' expected");

    lua_pushstring(L, ((luna_user *)ud)->level);
    return 1;
}


int
luaX_user_setlevel(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.user");
    const char *level = luaL_checkstring(L, 2);
    luna_user *target = NULL;

    luaL_argcheck(L, ud != NULL, 1, "'luna.user' expected");

    if ((target = find_user_by_userdata(L, ud)) != NULL)
    {
        strncpy(target->level, level, sizeof(target->level));
        memcpy(ud, target, sizeof(luna_user));
    }
    else
    {
        return luaL_error(L, "no such user '%s'", ((luna_user *)ud)->hostmask);
    }
    
    return 0;
}


int
luaX_user_getid(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.user");
    luaL_argcheck(L, ud != NULL, 1, "'luna.user' expected");

    lua_pushstring(L, ((luna_user *)ud)->id);
    return 1;
}


int
luaX_user_setid(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "luna.user");
    const char *id = luaL_checkstring(L, 2);
    luna_user *target = NULL;

    luaL_argcheck(L, ud != NULL, 1, "'luna.user' expected");

    if ((target = find_user_by_userdata(L, ud)) != NULL)
    {
        strncpy(target->id, id, sizeof(target->id));
        memcpy(ud, target, sizeof(luna_user));
    }
    else
    {
        return luaL_error(L, "no such user '%s'", ((luna_user *)ud)->hostmask);
    }
    
    return 0;
}


int
luaX_free_user(lua_State *L)
{
    /* Nothing to do yet */
    return 0;
}


int
luaX_users_find(lua_State *L)
{
    luna_state *state = NULL;

    const char *nick = NULL;
    const char *user = NULL;
    const char *host = NULL;

    luaL_checktype(L, 1, LUA_TTABLE);

    state = api_getstate(L);

    lua_pushstring(L, "nick");
    lua_gettable(L, 1);
    nick = lua_tostring(L, lua_gettop(L));

    lua_pushstring(L, "user");
    lua_gettable(L, 1);
    user = lua_tostring(L, lua_gettop(L));

    lua_pushstring(L, "host");
    lua_gettable(L, 1);
    host = lua_tostring(L, lua_gettop(L));

    if ((nick && strcmp(nick, "")) &&
        (user && strcmp(user, "")) &&
        (host && strcmp(host, "")))
    {
        /* Copy values over */
        irc_sender tmp;
        void *data = NULL;
        void *key = (void *)&tmp;

        strncpy(tmp.nick, nick, sizeof(tmp.nick) - 1);
        strncpy(tmp.user, user, sizeof(tmp.user) - 1);
        strncpy(tmp.host, host, sizeof(tmp.host) - 1);

        if ((data = list_find(state->users, key, &luna_user_cmp)) != NULL)
        {
            luna_user *u = (luna_user *)data;
            luna_user *ud = (luna_user *)lua_newuserdata(L, sizeof(luna_user));

            luaL_getmetatable(L, "luna.user");
            lua_setmetatable(L, -2);

            memcpy(ud, u, sizeof(luna_user));
        }
        else
        {
            lua_pushnil(L);
        }

        return 1;
    }
    else
    {
        return luaL_error(L, "fields 'nick', 'user' and 'host' required");
    }

    return 0;
}


int
luaX_users_getall(lua_State *L)
{
    int array;
    int i = 1;
    luna_state *state;
    list_node *cur = NULL;

    state = api_getstate(L);

    lua_newtable(L);
    array = lua_gettop(L);

    for (cur = state->users->root; cur != NULL; cur = cur->next)
    {
        luna_user *ud = (luna_user *)lua_newuserdata(L, sizeof(luna_user));
        luaL_getmetatable(L, "luna.user");
        lua_setmetatable(L, -2);

        memcpy(ud, cur->data, sizeof(luna_user));

        lua_rawseti(L, array, i++);
    }

    return 1;
}


int
luaX_users_add(lua_State *L)
{
    const char *id    = luaL_checkstring(L, 1);
    const char *mask  = luaL_checkstring(L, 2);
    const char *flags = luaL_checkstring(L, 3);
    const char *level = luaL_checkstring(L, 4);

    luna_state *state = api_getstate(L);

    users_add(state, id, mask, flags, level);
    users_write(state, "users.txt");

    return 0;
}


int
luaX_users_remove(lua_State *L)
{
    const char *mask = luaL_checkstring(L, 1);

    luna_state *state = api_getstate(L);

    users_remove(state, mask);
    users_write(state, "users.txt");

    return 0;
}


int
luaX_users_save(lua_State *L)
{
    luna_state *state = api_getstate(L);

    users_write(state, "users.txt");

    return 0;
}


int
luaX_users_reload(lua_State *L)
{
    luna_state *state = api_getstate(L);

    users_reload(state, "users.txt");

    return 0;
}


int
luaX_register_user(lua_State *L, int regtable)
{
    int meta = (luaL_newmetatable(L, "luna.user"), lua_gettop(L));

    /* Register garbage collector function */
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, luaX_free_user);
    lua_settable(L, meta);

    /* Register indexing field */
    lua_pushstring(L, "__index");
    lua_pushvalue(L, meta);
    lua_settable(L, meta);

    /* Register all methods within this table */
    luaL_openlib(L, NULL, luaX_user_methods, 0);

    /* And register other functions inside regtable
     * luna.users = { ... } */
    lua_pushstring(L, "users");
    lua_newtable(L);
    luaL_register(L, NULL, luaX_user_functions);
    lua_settable(L, regtable);

    return 1;
}
