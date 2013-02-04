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

#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../lua_util.h"


int luaX_channel_getchannels(lua_State *);
int luaX_channel_getchannelinfo(lua_State *);
int luaX_channel_getchannelusers(lua_State *);
int luaX_channel_getchanneluserinfo(lua_State *);

static const struct luaL_Reg luaX_channel_functions[] =
{
    { "get_channels", luaX_channel_getchannels },
    { "get_channel_info", luaX_channel_getchannelinfo },
    { "get_channel_users", luaX_channel_getchannelusers },
    { "get_channel_user_info", luaX_channel_getchanneluserinfo },

    { NULL, NULL }
};


const char *getaddr(irc_user *user)
{
    /* TODO: :( */
    const size_t n = NICKLEN + IDENTLEN + HOSTLEN + 1;
    static char addr[NICKLEN + IDENTLEN + HOSTLEN];

    snprintf(addr, n, "%s!%s@%s", user->nick, user->user, user->host);

    return addr;
}

int luaX_push_modes(lua_State *L, irc_channel *channel)
{
    int max = 64; /* TODO: Calculate */
    int i;
    int table = (lua_newtable(L), lua_gettop(L));

    for (i = 0; i < max; ++i)
    {
        if (channel->flags[i].set)
        {
            list_node *cur = NULL;
            int list;

            flag *f = &(channel->flags[i]);

            lua_pushfstring(L, "%c", i + 'A');

            switch (f->type)
            {
            case FLAG_NONE:
                lua_pushboolean(L, 1);
                break;

            case FLAG_STRING:
                lua_pushstring(L, f->string);
                break;

            case FLAG_LIST:
                list = (lua_newtable(L), lua_gettop(L));

                for (cur = f->list->root; cur != NULL; cur = cur->next)
                {
                    lua_pushstring(L, cur->data);
                    lua_rawseti(L, list, i++);
                }

                break;
            }

            lua_settable(L, table);
        }
    }

    return 1;
}

int luaX_push_channelinfo(lua_State *L, irc_channel *channel)
{
    int table = (lua_newtable(L), lua_gettop(L));

    lua_pushstring(L, "name");
    lua_pushstring(L, channel->name);
    lua_settable(L, table);

    lua_pushstring(L, "topic");
    lua_pushstring(L, channel->topic);
    lua_settable(L, table);

    lua_pushstring(L, "topic_set_by");
    lua_pushstring(L, channel->topic_setter);
    lua_settable(L, table);

    lua_pushstring(L, "topic_set_at");
    lua_pushnumber(L, channel->topic_set);
    lua_settable(L, table);

    lua_pushstring(L, "created");
    lua_pushnumber(L, channel->created);
    lua_settable(L, table);

    lua_pushstring(L, "modes");
    luaX_push_modes(L, channel);
    lua_settable(L, table);

    return 1;
}

int luaX_push_channeluserinfo(lua_State *L, irc_user *user)
{
    int table = (lua_newtable(L), lua_gettop(L));

    lua_pushstring(L, "modes");
    lua_pushstring(L, user->modes);
    lua_settable(L, table);

    return 1;
}

int luaX_channel_getchannels(lua_State *L)
{
    int arr;
    int i = 1;
    list_node *cur;

    luna_state *state = api_getstate(L);

    arr = (lua_newtable(L), lua_gettop(L));
    for (cur = state->channels->root; cur != NULL; cur = cur->next)
    {
        lua_pushstring(L, ((irc_channel *)(cur->data))->name);
        lua_rawseti(L, arr, i++);
    }

    return 1;
}

int luaX_channel_getchannelinfo(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    irc_channel *result;

    luna_state *state = api_getstate(L);

    if ((result = list_find(state->channels, name, &channel_cmp)) != NULL)
        luaX_push_channelinfo(L, result);
    else
        return luaL_error(L, "no such channel '%s'", name);

    return 1;
}

int luaX_channel_getchannelusers(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    irc_channel *result = NULL;

    luna_state *state = api_getstate(L);

    if ((result = list_find(state->channels, name, &channel_cmp)) != NULL)
    {
        list_node *cur;
        int i = 1;
        int list = (lua_newtable(L), lua_gettop(L));

        for (cur = result->users->root; cur != NULL; cur = cur->next)
        {
            lua_pushstring(L, getaddr(cur->data));
            lua_rawseti(L, list, i++);
        }

        return 1;
    }
    else
    {
        return luaL_error(L, "no such channel '%s'", name);
    }
}

int luaX_channel_getchanneluserinfo(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    const char *nick = luaL_checkstring(L, 2);

    irc_user *result = NULL;

    if (strchr(nick, '!') != NULL)
        *(strchr(nick, '!')) = 0;

    luna_state *state = api_getstate(L);

    if ((result = list_find(state->channels, name, &channel_cmp)) != NULL)
    {
        irc_user *resuser = channel_get_user(state, name, nick);

        if (resuser)
            luaX_push_channeluserinfo(L, resuser);
        else
            lua_pushnil(L);

        return 1;
    }
    else
    {
        return luaL_error(L, "no such channel '%s'", name);
    }
}

int luaX_register_channel(lua_State *L, int regtable)
{
    /* Register functions inside regtable
     * luna.scripts = { ... } */
    lua_pushstring(L, "channels");

#if LUA_VERSION_NUM == 502
    luaL_newlib(L, luaX_channel_functions);
#else
    lua_newtable(L);
    luaL_register(L, NULL, luaX_channel_functions);
#endif

    lua_settable(L, regtable);

    return 1;
}
