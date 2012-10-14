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
 *  Bot state (state.c)
 *  ---
 *  Define state to be passed around between the various functions
 *
 *  Created: 25.02.2011 12:28:10
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "state.h"
#include "linked_list.h"
#include "channel.h"
#include "handlers.h"
#include "lua_api/lua_util.h"


int
state_init(luna_state *state)
{
    memset(state, 0, sizeof(*state));

    if (list_init(&(state->users)) != 0)
        return 1;

    if (list_init(&(state->scripts)) != 0)
        return 1;

    return 0;
}


int
state_destroy(luna_state *state)
{
    logger_destroy(state->logger);
    list_destroy(state->users,   &free);
    list_destroy(state->scripts, &script_free);

    free(state->chantypes);

    return 0;
}

