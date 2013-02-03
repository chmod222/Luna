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
 *  Main loop (bot.c)
 *  ---
 *  All the runtime action
 *
 *  Created: 25.02.2011 13:51:57
 *
 ******************************************************************************/
#include <time.h>
#include <signal.h>
#include <string.h>

#include <arpa/inet.h>

#include "irc.h"
#include "bot.h"
#include "logger.h"
#include "net.h"
#include "handlers.h"
#include "user.h"

#include "lua_api/lua_manager.h"
#include "lua_api/lua_util.h"


int *killswitch_ptr = NULL;

void exit_gracefully(int);
void luna_send_login(luna_state *);


int
luna_mainloop(luna_state *state)
{
    fd_set reads;
    int tries = RECONN_MAX;

    killswitch_ptr = &(state->killswitch);
    state->started = time(NULL);

    signal(SIGINT, exit_gracefully);

    /* Try to load base script */
    if (script_load(state, "base.lua") != 0)
    {
        logger_log(state->logger, LOGLEV_ERROR, "Failed to load base script");

        return 1;
    }

    if ((users_load(state, "users.txt") != 0) || !(state->users->length))
        logger_log(state->logger, LOGLEV_WARNING, "Failed to load userlist! "
                   "You will be unable to load and unload scripts without "
                   "administration privileges!");
    else
        logger_log(state->logger, LOGLEV_INFO, "Loaded %d users.",
                   state->users->length);

    while (!(state->killswitch))
    {
        /* Did we max out all connection attempts? */
        if (!tries)
            break;

        if (net_connect(state) < 0)
        {
            logger_log(state->logger, LOGLEV_ERROR, "Failed to connect #%d",
                       (RECONN_MAX - tries) + 1);

            tries--;
            continue;
        }

        list_init(&(state->channels));

        tries = RECONN_MAX;
        state->connected = time(NULL);
        time_t last_sign_of_life = time(NULL);

        logger_log(state->logger, LOGLEV_INFO, "Connected! Sending login.");

        luna_send_login(state);

        /* Enter connection loop */
        while (!(state->killswitch))
        {
            struct timeval timeout;
            int ready;

            /* Wait a quarter of a second before emitting the "idle" signals */
            timeout.tv_sec  = 0;
            timeout.tv_usec = 250000;

            FD_ZERO(&reads);
            FD_SET(state->fd, &reads);

            ready = select(state->fd + 1, &reads, NULL, NULL, &timeout);

            if ((ready > 0) && (FD_ISSET(state->fd, &reads)))
            {
                /* We got data! */
                irc_event ev;
                char current_line[LINELEN];

                memset(current_line, 0, sizeof(current_line));

                if (net_recvln(state, current_line, sizeof(current_line)) < 0)
                    break;

                logger_log(state->logger, LOGLEV_INFO,  ">> %s", current_line);
                last_sign_of_life = time(NULL);
                irc_init_irc_event(&ev);
                irc_parse_line(&ev, current_line);

                /* Handle events */
                handle_event(state, &ev);

                /* Cleanup */
                irc_free_irc_event(&ev);
            }
            else
            {
                /* Handle idle event */

                /* Check if we're alive if the last event was TIMEOUT seconds
                 * ago, and reconnect if there was an error */
                if ((time(NULL) - last_sign_of_life) > TIMEOUT)
                    if (net_sendfln(state, "PING :%s",
                                    state->serverinfo.host) <= 0)
                        break;

                signal_dispatch(state, "idle", NULL);
            }
        }

        signal_dispatch(state, "disconnect", NULL);

        list_destroy(state->channels, &channel_free);
        net_disconnect(state);
    }

    return 0;
}


void
exit_gracefully(int sig)
{
    *killswitch_ptr = 1;

    return;
}


void
luna_send_login(luna_state *state)
{
    net_sendfln(state, "NICK %s", state->userinfo.nick);
    net_sendfln(state, "USER %s * 0 :%s",
                state->userinfo.user,
                state->userinfo.real);

    return;
}
