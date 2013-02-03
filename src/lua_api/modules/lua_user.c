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
 *  Provide meta info about registered users to scripts
 *
 *  Created: 03.02.2012 19:37:01
 *
 ******************************************************************************/
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../lua_util.h"

int luaX_user_getusers(lua_State *);
int luaX_user_getuserinfo(lua_State *);
int luaX_user_setuserinfo(lua_State *);
int luaX_user_matchuser(lua_State *);
int luaX_user_deleteuser(lua_State *);
int luaX_user_adduser(lua_State *);

static const struct luaL_Reg luaX_channel_functions[] =
{
    { "get_users", luaX_user_getusers },
    { "get_user_info", luaX_user_getuserinfo },
    { "set_user_info", luaX_user_setuserinfo },
    { "match_user", luaX_user_matchuser },
    { "delete_user", luaX_user_deleteuser },
    { "add_user", luaX_user_adduser },

    { NULL, NULL }
};


int
luaX_push_userinfo(lua_State *L, luna_user *u)
{
    int table = (lua_newtable(L), lua_gettop(L));

    lua_pushstring(L, "hostmask");
    lua_pushstring(L, u->hostmask);
    lua_settable(L, table);

    lua_pushstring(L, "flags");
    lua_pushstring(L, u->flags);
    lua_settable(L, table);

    lua_pushstring(L, "level");
    lua_pushstring(L, u->level);
    lua_settable(L, table);

    lua_pushstring(L, "id");
    lua_pushstring(L, u->id);
    lua_settable(L, table);

    return 1;
}


int
luaX_user_getusers(lua_State *L)
{
    int arr;
    int i = 1;
    list_node *cur;

    luna_state *state = api_getstate(L);

    arr = (lua_newtable(L), lua_gettop(L));
    for (cur = state->users->root; cur != NULL; cur = cur->next)
    {
        lua_pushstring(L, ((luna_user *)(cur->data))->id);
        lua_rawseti(L, arr, i++);
    }

    return 1;
}


int
luaX_user_getuserinfo(lua_State *L)
{
    const char *id = luaL_checkstring(L, 1);
    luna_user *result;
    luna_state *state = api_getstate(L);

    if ((result = list_find(state->users, id, &luna_user_id_cmp)) != NULL)
        luaX_push_userinfo(L, result);
    else
        return luaL_error(L, "no such user '%s'", id);

    return 1;
}


int
luaX_user_setuserinfo(lua_State *L)
{
    const char *id = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    luna_user *result;
    luna_state *state = api_getstate(L);

    if ((result = list_find(state->users, id, &luna_user_id_cmp)) != NULL)
    {
        const char *id = (lua_pushstring(L, "id"),
                          lua_gettable(L, 1),
                          lua_tostring(L, -1));
        const char *hostmask = (lua_pushstring(L, "hostmask"),
                                lua_gettable(L, 1),
                                lua_tostring(L, -1));
        const char *flags = (lua_pushstring(L, "flags"),
                             lua_gettable(L, 1),
                             lua_tostring(L, -1));
        const char *level = (lua_pushstring(L, "level"),
                             lua_gettable(L, 1),
                             lua_tostring(L, -1));

        if (id && hostmask && flags && level && strlen(id) &&
            strlen(hostmask) && strlen(flags) && strlen(level))
        {
            strncpy(result->id, id, sizeof(result->id));
            strncpy(result->hostmask, hostmask, sizeof(result->hostmask));
            strncpy(result->flags, flags, sizeof(result->flags));
            strncpy(result->level, level, sizeof(result->level));

            users_write(state, "users.txt");

            return 0;
        }
        else
        {
            return luaL_error(L, "incomplete user info table");
        }
    }
    else
    {
        return luaL_error(L, "no such user '%s'", id);
    }

    return 1;
}


int
luaX_user_matchuser(lua_State *L)
{
    const char *addr = luaL_checkstring(L, 1);
    luna_state *state = api_getstate(L);

    luna_user *user = NULL;

    if ((user = list_find(state->users, addr, &luna_user_cmp)) != NULL)
        lua_pushstring(L, user->id);
    else
        lua_pushnil(L);

    return 1;
}


int
luaX_user_deleteuser(lua_State *L)
{
    const char *id = luaL_checkstring(L, 1);

    luna_state *state = api_getstate(L);

    lua_pushboolean(L, !users_remove(state, id));
    users_write(state, "users.txt");

    return 1;
}


int
luaX_user_adduser(lua_State *L)
{
    luna_state *state = api_getstate(L);

    const char *id = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    const char *hostmask = (lua_pushstring(L, "hostmask"),
                            lua_gettable(L, 2),
                            lua_tostring(L, -1));

    const char *flags = (lua_pushstring(L, "flags"),
                         lua_gettable(L, 2),
                         lua_tostring(L, -1));

    const char *level = (lua_pushstring(L, "level"),
                         lua_gettable(L, 2),
                         lua_tostring(L, -1));

   if (id && hostmask && flags && level && strlen(id) &&
       strlen(hostmask) && strlen(flags) && strlen(level))

      lua_pushboolean(L, !users_add(state, id, hostmask, flags, level));
   else
       return luaL_error(L, "invalid user info table");

   users_write(state, "users.txt");

   return 1;
}


int
luaX_register_user(lua_State *L, int regtable)
{
    /* Register functions inside regtable
     * luna.scripts = { ... } */
    lua_pushstring(L, "regusers");

    luaL_newlib(L, luaX_channel_functions);
    lua_settable(L, regtable);

    return 1;
}
