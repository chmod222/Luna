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

#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "lua_manager.h"
#include "lua_util.h"

#include "../irc.h"


int api_loglevel_from_string(const char *lev)
{
    if (!lev)
        return LOGLEV_ERROR;

    if (!strcasecmp(lev, "debug"))
        return LOGLEV_DEBUG;
    else if (!strcasecmp(lev, "info"))
        return LOGLEV_INFO;
    else if (!strcasecmp(lev, "warning"))
        return LOGLEV_WARNING;
    else
        return LOGLEV_ERROR;
}

luna_state *api_getstate(lua_State *L)
{
    luna_state *state = NULL;

    lua_pushlightuserdata(L, (void *)env_key);
    lua_gettable(L, LUA_REGISTRYINDEX);

    state = lua_touserdata(L, -1);
    lua_pop(L, 1);

    return state;
}

int luaX_push_args(lua_State *L, int n, char **param)
{
    int i;
    int table = (lua_newtable(L), lua_gettop(L));

    for (i = 0; i < n; ++i)
    {
        lua_pushstring(L, param[i]);
        lua_rawseti(L, table, i + 1);
    }

    return 1;
}

int luaX_push_raw(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_command);
    luaX_push_args(L, ev->m_paramcount, ev->m_params);
    lua_pushstring(L, ev->m_msg);

    return 4;
}

int luaX_push_privmsg(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, ev->m_msg);

    return 3;
}

int luaX_push_ctcp(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);
    const char *ctcp = va_arg(args, const char *);
    const char *msg = va_arg(args, const char *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, ctcp);
    lua_pushstring(L, msg);

    return 4;
}

int luaX_push_ctcp_rsp(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);
    const char *ctcp = va_arg(args, const char *);
    const char *msg = va_arg(args, const char *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, ctcp);
    lua_pushstring(L, msg);

    return 4;
}

int luaX_push_action(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);
    const char *msg = va_arg(args, const char *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, msg);

    return 3;
}

int luaX_push_command(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);
    const char *cmd = va_arg(args, const char *);
    const char *rest = va_arg(args, const char *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, cmd);
    lua_pushstring(L, rest);

    return 4;
}

int luaX_push_join(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);

    return 2;
}

int luaX_push_join_sync(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_params[1]);

    return 1;
}

int luaX_push_part(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, ev->m_msg);

    return 3;
}

int luaX_push_quit(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_msg);

    return 2;
}

int luaX_push_notice(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, ev->m_msg);

    return 3;
}

int luaX_push_nick(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);
    const char *newnick = va_arg(args, const char *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, newnick);

    return 2;
}

int luaX_push_invite(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);
    const char *target = va_arg(args, const char *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, target);

    return 2;
}

int luaX_push_topic(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, ev->m_msg);

    return 3;
}

int luaX_push_kick(luna_state *st, lua_State *L, va_list args)
{
    irc_message *ev = va_arg(args, irc_message *);

    lua_pushstring(L, ev->m_prefix);
    lua_pushstring(L, ev->m_params[0]);
    lua_pushstring(L, ev->m_params[1]);
    lua_pushstring(L, ev->m_msg);

    return 4;
}

int luaX_push_script_load(luna_state *st, lua_State *L, va_list args)
{
    const char *name = va_arg(args, const char *);

    lua_pushstring(L, name);
    return 1;
}

int luaX_push_script_unload(luna_state *st, lua_State *L, va_list args)
{
    const char *name = va_arg(args, const char *);

    lua_pushstring(L, name);
    return 1;
}
