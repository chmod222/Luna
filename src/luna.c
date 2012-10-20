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
 *  Program entrance point (luna.c)
 *  ---
 *  Provides the entrance point to the program, handle command line arguments.
 *
 *  Created: 25.02.2011 10:42:33
 *
 ******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "luna.h"
#include "logger.h"
#include "state.h"
#include "config.h"
#include "bot.h"

void print_usage(const char *);
void log_session_start(luna_log *);
void log_session_end(luna_log *);


void
set_environment()
{
    /*
     * Set some environment variables that scripts might use
     */
    setenv("LUNA_NAME", PACKAGE_NAME, 1);
    setenv("LUNA_VERSION", PACKAGE_VERSION, 1);
    setenv("LUNA_FULL_VERSION", PACKAGE_STRING, 1);

    setenv("__DATE__", __DATE__, 1);
    setenv("__TIME__", __TIME__, 1);
}

int
main(int argc, char **argv)
{
    char config_file[FILENAMELEN] = "config.lua"; /* Default config file */
    char log_file[FILENAMELEN]    = "luna.log";   /* Default log file */
    int opt;

    /* Initialize logger */
    luna_state state;
    luna_log *log = NULL;

    set_environment();

    if (state_init(&state) != 0)
    {
        fprintf(stderr, "Unable to initialize state\n");
        return 1;
    }

    /* Parse command line arguments */
    while ((opt = getopt(argc, argv, "hc:l:")) >= 0)
    {
        switch (opt)
        {
        case 'h':
            print_usage(argv[0]);
            return 0;

        case 'c':
            memset(config_file, 0, sizeof(config_file));
            strncpy(config_file, optarg, sizeof(config_file) - 1);

            break;

        case 'l':
            memset(log_file, 0, sizeof(log_file));
            strncpy(log_file, optarg, sizeof(log_file) - 1);

            break;
        }
    }

    /* Try creating logger */
    if (logger_init(&log, log_file) != 0)
    {
        /* Failure, abort */
        perror("error: Unable to create logger -- logger_init()");
        state_destroy(&state);

        return EXIT_FAILURE;
    }

    state.logger = log;

    /* Read configuration file */
    if (config_load(&state, config_file) != 0)
    {
        fprintf(stderr, "error: Unable to read configuration file -- abort\n");
        state_destroy(&state);

        return EXIT_FAILURE;
    }

    log_session_start(log);

    logger_log(log, LOGLEV_INFO, "Configuration: %s!%s (%s) -> %s:%d",
               state.userinfo.nick,
               state.userinfo.user,
               state.userinfo.real,
               state.serverinfo.host,
               state.serverinfo.port);

    logger_log(log, LOGLEV_INFO, "Entering main loop");
    luna_mainloop(&state);
    logger_log(log, LOGLEV_INFO, "Exiting...");

    log_session_end(log);
    state_destroy(&state);

    return EXIT_SUCCESS;
}


void
print_usage(const char *program)
{
    printf("Usage: %s [FLAGS..]\n\n", program);
    printf("  -h           display this help\n");
    printf("  -l <FILE>    log to FILE\n");
    printf("  -c <FILE>    use configuration file FILE instead of `config.lua'\n");

    return;
}


void
log_session_start(luna_log *log)
{
    logger_log(log, LOGLEV_INFO, "Starting session");

    return;
}


void
log_session_end(luna_log *log)
{
    logger_log(log, LOGLEV_INFO, "Stopping session");
    logger_log(log, LOGLEV_INFO, "---------------------------");

    return;
}
