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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "irc.h"
#include "net.h"
#include "util.h"
#include "mm.h"


irc_parse_status irc_parse_message(const char *line, irc_message *msg)
{
    const char *prev = line;
    const char *next = line;
    size_t len;

    memset(msg, 0, sizeof(*msg));

    /* Check for prefix */
    if (*prev == ':')
    {
        ++prev; /* skip the ':' */

        /* Prefix must not be the last part in a message */
        if ((next = strchr(prev, ' ')) == NULL)
            return SINVALID;

        /* Calculate token lenght and skip the space */
        len = next - prev;
        for (;*next == ' '; ++next);

        if ((msg->m_prefix = xstrndup(prev, len)) == NULL)
            return SNOMEM;

        prev = next;
    }

    /* Command also must not be the last argument in a message */
    if ((next = strchr(next, ' ')) == NULL)
        return SINVALID;

    len = next - prev;
    for (;*next == ' '; ++next);

    if ((msg->m_command = xstrndup(prev, len)) == NULL)
        return SNOMEM;

    do
    {
        prev = next;
        next = strchr(next, ' ') != NULL
            ? strchr(next, ' ')
            : next + strlen(next);
        len = next - prev;

        for (;*next == ' '; ++next);

        if (*prev != ':')
        {
            char **nparams = (char **)mm_realloc(msg->m_params,
                    sizeof(char*) * (msg->m_paramcount + 1));
            char *narg = xstrndup(prev, len);

            if (!nparams || !narg)
                return SNOMEM;

            msg->m_params = nparams;
            msg->m_params[msg->m_paramcount++] = narg;
        }
        else
        {
            ++prev;
            len = strlen(prev);

            msg->m_msg = xstrndup(prev, len);

            break;
        }
    }
    while (next < line + strlen(line));

    return SOK;
}

void irc_free_message(irc_message *msg)
{
    int i;

    mm_free(msg->m_prefix);
    mm_free(msg->m_command);

    for (i = 0; i < msg->m_paramcount; ++i)
        mm_free(msg->m_params[i]);

    mm_free(msg->m_params);
    mm_free(msg->m_msg);
}

void irc_print_message(struct irc_message *i)
{
    int j;

    if (i->m_prefix != NULL)
        printf(":%s ", i->m_prefix);

    printf("%s ", i->m_command);

    for (j = 0; j < i->m_paramcount; j++)
        printf("%s ", i->m_params[j]);

    if (i->m_msg != NULL)
        printf(":%s", i->m_msg);

    printf("\n");
}

int irc_user_cmp(const char *a, const char *b)
{
    /*
     * Since nicks are enough to identify someone uniquely on a network,
     * we can skip the whole address part and compare complete prefixes
     * only until the end of the nick part.
     */
    int reallen_a = strchr(a, '!') ? (strchr(a, '!') - a) : strlen(a);
    int reallen_b = strchr(b, '!') ? (strchr(b, '!') - b) : strlen(b);
    int cmplen = reallen_a > reallen_b ? reallen_b : reallen_a;

    if (reallen_a != reallen_b)
        return 1;
    else
        return strncasecmp(a, b, cmplen);
}
