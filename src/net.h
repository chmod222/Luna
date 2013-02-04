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

#ifndef NET_H
#define NET_H

#include "state.h"

#define LINELEN 512

int net_connect(luna_state *);
int net_disconnect(luna_state *);

int net_sendfln(luna_state *, const char *, ...);
int net_vsendfln(luna_state *, const char *, va_list);
int net_recvln(luna_state *, char *dest, size_t len);

#endif
