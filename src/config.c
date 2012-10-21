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
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "state.h"
#include "logger.h"
#include "mm.h"


int config_get_userinfo(luna_state *, lua_State *);
int config_get_serverinfo(luna_state *, lua_State *);
int config_get_netinfo(luna_state *, lua_State *);


int
config_load(luna_state *state, const char *filename)
{
    lua_State *L = NULL;

    /* Create lua environment */
    if ((L = lua_newstate(&mm_lalloc, NULL)))
    {
        int status = 0;

        if (luaL_dofile(L, filename) == 0)
        {
            /* Both user and server must be set (== 0) */
            if (config_get_userinfo(state, L) || config_get_serverinfo(state, L))
                status = 1;
            else
                config_get_netinfo(state, L);
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
    const char *nick = NULL;
    const char *user = NULL;
    const char *real = NULL;

    lua_getglobal(L, "nick");
    nick = lua_tostring(L, lua_gettop(L));

    lua_getglobal(L, "user");
    user = lua_tostring(L, lua_gettop(L));

    lua_getglobal(L, "realname");
    real = lua_tostring(L, lua_gettop(L));

    /* Nick and user can not be empty */
    if ((nick && (strcmp("", nick))) && (user && (strcmp("", user))))
    {
        /* Userinfo okay */
        strncpy(state->userinfo.nick, nick, sizeof(state->userinfo.nick) - 1);
        strncpy(state->userinfo.user, user, sizeof(state->userinfo.user) - 1);

        if (real)
            strncpy(state->userinfo.real, real,
                    sizeof(state->userinfo.real) - 1);

        return 0;
    }

    return 1;
}


int
config_get_serverinfo(luna_state *state, lua_State *L)
{
    const char *host = NULL;

    lua_getglobal(L, "server");
    host = lua_tostring(L, lua_gettop(L));

    /* Server nonempty string, port may be ommited (=6667) */
    if (host && (strcmp("", host)))
    {
        strncpy(state->serverinfo.host, host,
                sizeof(state->serverinfo.host) - 1);

        lua_getglobal(L, "port");

        if (lua_type(L, lua_gettop(L)) == LUA_TNUMBER)
            state->serverinfo.port = lua_tonumber(L, lua_gettop(L));
        else
            state->serverinfo.port = 6667;

        return 0;
    }

    return 1;
}


int
config_get_netinfo(luna_state *state, lua_State *L)
{
    const char *bind = NULL;

    lua_getglobal(L, "bind");
    bind = lua_tostring(L, lua_gettop(L));

    if (bind && strcmp("", bind))
    {
        if ((state->bind = mm_malloc(strlen(bind) + 1)) == NULL)
        {
            logger_log(state->logger, LOGLEV_WARNING,
                       "Couldn't allocate memory for bind, keeping NULL");
        }
        else
        {
            strcpy(state->bind, bind);
        }
    }

    return 0;
}
