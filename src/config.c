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
 *  Configuration management (config.c)
 *  ---
 *  Read and apply configuration
 *
 *  Created: 25.02.2011 12:43:25
 *
 ******************************************************************************/
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "state.h"
#include "logger.h"


int config_get_userinfo(luna_state *, lua_State *);
int config_get_serverinfo(luna_state *, lua_State *);


int
config_load(luna_state *state, const char *filename)
{
    lua_State *L = NULL;

    /* Create lua environment */
    if ((L = lua_open()))
    {
        int status = 0;

        if (luaL_dofile(L, filename) == 0)
        {
            int check_user = 1;
            int check_server = 1;

            check_user   = config_get_userinfo(state, L);
            check_server = config_get_serverinfo(state, L);

            /* Both user and server must be set */
            if ((check_user != 0) || (check_server != 0))
                status = 1;
        }
        else
        {
            logger_log(state->logger, LOGLEV_ERROR, "Lua error: %s",
                       lua_tostring(L, -1));

            status = 1;
        }

        /* Cleanup */
        lua_close(L);

        return status;
    }

    return 1;
}


int
config_get_userinfo(luna_state *state, lua_State *L)
{
    int n; /* Nick */
    int u; /* User */
    int r; /* Realname */

    const char *nick = NULL;
    const char *user = NULL;
    const char *real = NULL;

    lua_getglobal(L, "nick");
    n = lua_gettop(L);

    lua_getglobal(L, "user");
    u = lua_gettop(L);

    lua_getglobal(L, "realname");
    r = lua_gettop(L);

    nick = lua_tostring(L, n);
    user = lua_tostring(L, u);
    real = lua_tostring(L, r);

    /* Nick and user can not be empty, real name can - all must be strings */
    if (((lua_type(L, n) == LUA_TSTRING) && (strcmp("", nick))) &&
        ((lua_type(L, u) == LUA_TSTRING) && (strcmp("", user))) //&&
      /*   ((lua_type(L, r) == LUA_TSTRING))*/)
    {
        /* Userinfo okay */
        strncpy(state->userinfo.nick, nick, sizeof(state->userinfo.nick) - 1);
        strncpy(state->userinfo.user, user, sizeof(state->userinfo.user) - 1);

        if (lua_type(L, r) == LUA_TSTRING)
            strncpy(state->userinfo.real, real,
                    sizeof(state->userinfo.real) - 1);

        return 0;
    }

    return 1;
}


int
config_get_serverinfo(luna_state *state, lua_State *L)
{
    int h; /* Host */
    int p; /* Port */

    const char *host = NULL;

    lua_getglobal(L, "server");
    h = lua_gettop(L);

    lua_getglobal(L, "port");
    p = lua_gettop(L);

    host = lua_tostring(L, h);

    /* Server nonempty string, port may be ommited (=6667) */
    if ((lua_type(L, h) == LUA_TSTRING) && (strcmp("", host)))
    {
        strncpy(state->serverinfo.host, host,
                sizeof(state->serverinfo.host) - 1);

        if (lua_type(L, p) == LUA_TNUMBER)
            state->serverinfo.port = lua_tonumber(L, p);
        else
            state->serverinfo.port = 6667;

        return 0;
    }

    return 1;
}
