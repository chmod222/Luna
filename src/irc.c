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

/******************************************************************************
 *
 *  IRC line parsing and utilities (irc.c)
 *  ---
 *  Parse IRC lines
 *
 *  Created: 25.02.2011 15:18:46
 *
 ******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "irc.h"
#include "net.h"
#include "util.h"


void free_stringtable(char **dest, size_t size);
static char *irc_type_strings[] =
{
    "???", "ERROR", "NUMERIC", "JOIN",
    "PART", "QUIT", "PRIVMSG", "NOTICE",
    "NICK", "MODE", "PING", "INVITE",
    "TOPIC", "KICK", NULL
};


int
irc_parse_line(struct irc_event *dest, char *line)
{
    char *arg_command, *arg_sender, *arg_params;
    char *nick, *user, *host;
    char *tmp;
    int i;

    if ((line == NULL) || (strcmp(line, "") == 0))
    {
        fprintf(stderr, "parse(): Empty input\n");

        return 1;
    }

    nick = NULL;
    user = NULL;
    host = NULL;

    tmp = strtok(line, " ");

    if (tmp[0] != ':')
    {
        /* we don't need to do complicated parsing here, just get the rest. */
        arg_sender = NULL;
        arg_command = tmp;
        arg_params = strtok(NULL, "");

        if ((arg_params == NULL) || (strcmp(arg_params, "") == 0))
        {
            fprintf(stderr, "parse(): Command without params\n");

            return 1;
        }
    }
    else if (strcmp(tmp, ":") != 0)
    {
        arg_sender = tmp;
        arg_command = strtok(NULL, " ");
        arg_params = strtok(NULL, "");

        if ((arg_command == NULL) || (arg_params == NULL) ||
           !(strcmp(arg_command, "")) || !(strcmp(arg_params, "")))
        {
            fprintf(stderr, "parse(): Incomplete line, missing either command "
                            "or arguments.\n");

            return 1;
        }
    }
    else
    {
        fprintf(stderr, "parse(): Unable to parse line\n");

        return 1;
    }

    if (arg_sender != NULL)
    {
        arg_sender++; /* Remove ':' at the beginning */

        if ((strstr(arg_sender, "!") != NULL) &&
            (strstr(arg_sender, "@") != NULL))
        {
            nick = strtok(arg_sender, "!");
            user = strtok(NULL, "@");
            host = strtok(NULL, "");

            strncpy(dest->from.nick, nick, sizeof(dest->from.nick) - 1);
            strncpy(dest->from.user, user, sizeof(dest->from.user) - 1);
            strncpy(dest->from.host, host, sizeof(dest->from.host) - 1);
        }
        else
        {
            /*host = arg_sender;*/

            strcpy(dest->from.nick, "");
            strcpy(dest->from.user, "");

            strncpy(dest->from.host, arg_sender, sizeof(dest->from.host) - 1);

        }
    }
    else
    {
        strcpy(dest->from.nick, "");
        strcpy(dest->from.user, "");
        strcpy(dest->from.host, "");
    }

    dest->numeric = atoi(arg_command);

    if (dest->numeric != 0)
        dest->type = IRCEV_NUMERIC;
    else
        dest->type = irc_event_type_from_string(arg_command);

    dest->param = (char **)malloc(sizeof(char *));
    tmp = strtok(arg_params, " "); /* get first argument */

    dest->msg = NULL;

    for (i = 0; (tmp != NULL); i++)
    {
        if (tmp[0] == ':')
        {
            char *j;

            char msg[MSGLEN];
            char *rest = strtok(NULL, "");

            strncpy(msg, ++tmp, MSGLEN);

            if (rest != NULL)
            {
                strncat(msg, " ", MSGLEN);
                strncat(msg, rest, MSGLEN);
            }

            j = msg + strlen(msg) - 1;
            while ((j >= msg) && isspace(*j))
                j--;

            *(j+1) = 0;

            dest->msg = (char *)malloc(strlen(msg) + 1);

            if (dest->msg == NULL)
            {
                fprintf(stderr, "parse(): Couldn't allocate memory "
                                "for message\n");

                free(dest->msg);

                free_stringtable(dest->param, dest->param_count);

                return 1;
            }
            else
            {
                strncpy(dest->msg, msg, strlen(msg));
                dest->msg[strlen(msg)] = 0;
            }

            break;
        }

        dest->param[i] = (char *)malloc(strlen(tmp) + 1);
        dest->param = (char **)realloc(dest->param, sizeof(char *) * (i+2));

        if ((dest->param[i] == NULL) ||
                (dest->param == NULL))
        {
            fprintf(stderr, "parse(): Couldn't allocate memory for arg #%d\n",
                    i+1);

            free_stringtable(dest->param, dest->param_count);

            return 1;
        }
        else
        {
            strncpy(dest->param[i], tmp, strlen(tmp));
            dest->param[i][strlen(tmp)] = 0;
        }


        tmp = strtok(NULL, " ");
    }

    dest->param_count = i;
    dest->param[i] = NULL;

    return 0;
}


void
free_stringtable(char **t, size_t size)
{
    int i;

    for (i = 0; i < size; i++)
        free(t[i]);

    free(t);
}


void
irc_free_irc_event(struct irc_event *d)
{
    free(d->msg);

    free_stringtable(d->param, d->param_count);
}


void
irc_init_irc_event(struct irc_event *d)
{
    strcpy(d->from.nick, "");
    strcpy(d->from.user, "");
    strcpy(d->from.host, "");

    d->type = 0;
    d->msg = NULL;
    d->param = NULL;
    d->param_count = 0;
}


void
irc_print_irc_event(struct irc_event *i)
{
    int j;

    if (strcmp(i->from.host, "") != 0)
    {
        if (strcmp(i->from.user, "") != 0)
            printf(":%s!%s@%s ",
                    i->from.nick,
                    i->from.user,
                    i->from.host);
        else
            printf(":%s ", i->from.host);

    }

    printf("%s ", irc_event_type_to_string(i->type));
    if (i->type == IRCEV_NUMERIC)
        printf("(%d) ", i->numeric);

    for (j = 0; j < i->param_count; j++)
        printf("%s ", i->param[j]);

    if (i->msg != NULL)
        printf(":%s", i->msg);

    printf("\n");
}


char *
irc_event_type_to_string(enum irc_event_type t)
{
    int i;

    for (i = 0; irc_type_strings[i] != NULL; ++i)
        if (i == t)
            return irc_type_strings[i];

    return NULL;
}


enum irc_event_type
irc_event_type_from_string(const char *s)
{
    int i;

    for (i = 0; irc_type_strings[i] != NULL; ++i)
        if (strcmp(irc_type_strings[i], s) == 0)
            return i;

    return 0;
}

