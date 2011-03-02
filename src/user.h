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
 *  User management (user.h)
 *  ---
 *  Manage the userlist
 *
 *  Created: 26.02.2011 17:37:26
 *
 ******************************************************************************/
#ifndef USER_H
#define USER_H

#include "linked_list.h"
#include "irc.h"

typedef struct luna_user
{
    char id[64];
    char hostmask[128];
    char flags[64];
    char level[64];
} luna_user;


int users_load(luna_state *, const char *);
int users_unload(luna_state *);
int users_reload(luna_state *, const char *);
int users_write(luna_state *, const char *);

int users_add(luna_state *, const char *, const char *,
              const char *, const char *);
int users_remove(luna_state *, const char *);

int luna_user_cmp(void *, void *);
int luna_user_host_cmp(void *, void *);

luna_user *user_match(luna_state *, irc_sender *);

int strwcmp(const char *, const char *);
int strwcasecmp(const char *, const char *);

#endif
