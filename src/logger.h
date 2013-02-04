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

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#include "luna.h"

typedef enum luna_loglevel
{
    LOGLEV_INFO,
    LOGLEV_WARNING,
    LOGLEV_ERROR
} luna_loglevel;

typedef struct luna_log
{
    char filename[FILENAMELEN];

    FILE *file;
} luna_log;


int logger_init(luna_log **, const char *);
int logger_destroy(luna_log *);

int logger_log(luna_log *, luna_loglevel, const char *, ...);

#endif
