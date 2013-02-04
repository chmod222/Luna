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

#ifndef LUNA_H
#define LUNA_H

/* Enable or disable debugging */
#define DEBUG 1

/* Enable or disable memory debugging (spammy) */
/* #define MMDEBUG 1 */

/* Maximum file name length */
#define FILENAMELEN 128

/* Log format */
#define LOG_FORMAT "%s (%s): " /* First param = stamp, second = level */

/* Timestamps */
#define TIMESTAMP_FORMAT "%a %d. %b %T %Z %Y"

/* Use localtime - undef for GMT */
#define TIMESTAMP_LOCAL

/* Reconnection attempts. TODO: read it from config.lua */
#define RECONN_MAX 5

/* Seconds of inactivity after which the connection is assumed dead */
#define TIMEOUT 300

#endif
