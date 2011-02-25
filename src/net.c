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
 *  Network subsystem (net.c)
 *  ---
 *  Handle sending and receiving
 *
 *  Created: 25.02.2011 13:34:53
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "net.h"
#include "state.h"
#include "util.h"


int
net_connect(luna_state *state)
{
    struct addrinfo hints, *resolv, *p;
    int fd, res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((res = getaddrinfo(state->serverinfo.host, itoa(state->serverinfo.port),
                           &hints, &resolv)) != 0)
        return -1;

    for (p = resolv; p != NULL; p = p->ai_next)
    {
        /*
        char ipstring[INET6_ADDRSTRLEN] = { 0 };
        struct sockaddr *sa = (struct sockaddr *)p->ai_addr;

        inet_ntop(p->ai_family,
                (sa->sa_family == AF_INET)
                    ? (void *)&(((struct sockaddr_in *)sa)->sin_addr)
                    : (void *)&(((struct sockaddr_in6 *)sa)->sin6_addr),
                ipstring,
                sizeof(ipstring)); */

        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Couldn't create socket */

        if (connect(fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(fd);

            continue;
        }

        /* Connected, abort looping */
        break;
    }

    freeaddrinfo(resolv);

    /* Loop not terminated manually? */
    if (p == NULL)
        return -1;

    state->fd = fd;

    return fd;
}


int
net_disconnect(luna_state *state)
{
    return close(state->fd);
}


int
net_sendfln(luna_state *state, const char *format, ...)
{
    va_list args;
    char buffer[LINELEN];

    memset(buffer, 0, sizeof(buffer));

    /* Write at most LINELEN - 3 bytes to leave space for "\r\n\0" */
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer) - 3, format, args);
    va_end(args);

#ifdef DEBUG
    printf(">> %s\n", buffer);
#endif
    /* Safe, space was left by vsnprintf */
    strcat(buffer, "\r\n");

    return send(state->fd, buffer, strlen(buffer), 0);
}


int
net_recvln(luna_state *state, char *buffer, size_t len)
{
    char ch;
    int i;

    memset(buffer, 0, len);

    for (i = 0; i < len; i++)
    {
        int c = recv(state->fd, &ch, 1, 0);

        if (c > 0 )
        {
            if (ch == '\r')
                buffer[i] = 0;
            else if (ch == '\n')
            {
                buffer[i] = 0;

                return i+1;
            }
            else
                buffer[i] = ch;
        }  
        else
        {
            perror("recv()");

            return -1;
        }
    }

    return -1;
}
