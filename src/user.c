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
 *  User management (user.c)
 *  ---
 *  Manage the userlist
 *
 *  Created: 26.02.2011 17:36:49
 *
 ******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "user.h"
#include "linked_list.h"


int
users_load(linked_list *userlist, const char *file)
{
    FILE *userfile = fopen(file, "r");
    if (userfile)
    {
        char buffer[256];

        while (!feof(userfile))
        {
            char *mask  = NULL;
            char *level = NULL;

            memset(buffer, 0, sizeof(buffer));

            if (!fgets(buffer, sizeof(buffer), userfile))
                break;

            mask  = strtok(buffer, ":");
            level = strtok(strtok(NULL, ""), "\n");

            if (mask && level)
            {
                luna_user *user = malloc(sizeof(*user));

                if (user)
                {
                    memset(user, 0, sizeof(*user));

                    strncpy(user->hostmask, mask, sizeof(user->hostmask) - 1);
                    strncpy(user->level, level, sizeof(user->level) - 1);

                    list_push_back(userlist, user);
                }
            }
        }

        fclose(userfile);
        return 0;
    }

    return 1;
}


int
users_unload(linked_list *userlist)
{
    list_node *cur = NULL;

    for (cur = userlist->root; cur != NULL; cur = userlist->root)
        list_delete(userlist, cur->data, &free);

    return 0;
}


int
users_reload(linked_list *userlist, const char *file)
{
    users_unload(userlist);

    return users_load(userlist, file);
}


int
users_write(linked_list *userlist, const char *file)
{
    FILE *f = fopen(file, "w");

    if (f)
    {
        list_node *cur = NULL;

        for (cur = userlist->root; cur != NULL; cur = cur->next)
        {
            luna_user *u = (luna_user *)(cur->data);

            fprintf(f, "%s:%s\n", u->hostmask, u->level);
        }

        fclose(f);

        return 0;
    }

    return 1;
}


int
users_add(linked_list *userlist, const char *hostmask, const char *level)
{
    luna_user *user = malloc(sizeof(*user));

    if (user)
    {
        memset(user, 0, sizeof(*user));

        strncpy(user->hostmask, hostmask, sizeof(user->hostmask));
        strncpy(user->level, level, sizeof(user->level));

        list_push_back(userlist, user);
    }

    return 1;
}


int
users_remove(linked_list *userlist, const char *hostmask)
{
    void *user = list_find(userlist, (void *)hostmask, &luna_user_host_cmp);

    if (user)
    {
        list_delete(userlist, user, &free);
        return 0;
    }

    return 1;
}


int
luna_user_cmp(void *data, void *list_data)
{
    char host[256];

    luna_user *user = (luna_user *)list_data;
    irc_sender *key = (irc_sender *)data;

    memset(host, 0, sizeof(host));
    snprintf(host, sizeof(host), "%s!%s@%s", key->nick, key->user, key->host);

    return !(strwcasecmp(host, user->hostmask));
}


int
luna_user_host_cmp(void *data, void *list_data)
{
    char *key = (char *)data;
    luna_user *user = (luna_user *)list_data;

    return strcasecmp(key, user->hostmask);
}


int
user_match_level(linked_list *userlist, irc_sender *s, const char *level)
{
    void *user = list_find(userlist, (void *)s, &luna_user_cmp);
    
    if (user)
    {
        luna_user *user_real = (luna_user *)user;

        return strcasecmp(user_real->level, level);
    }

    return 1;
}


char *
user_get_level(linked_list *userlist, irc_sender *s)
{
    void *user = list_find(userlist, (void *)s, &luna_user_cmp);
    
    if (user)
    {
        luna_user *user_real = (luna_user *)user;

        return user_real->level;
    }

    return NULL;
}


int
strwcmp(const char *str, const char *pat)
{
    switch (*pat)
    {
        case 0:
            return !(*str);

        case '*':
            return (strwcmp(str, pat + 1) || (*str && strwcmp(str + 1, pat)));
                
        case '?':
            return (*str && strwcmp(str + 1, pat + 1));

        default:
            return ((*str == *pat) && (strwcmp(str + 1, pat + 1)));
    }

    return 0;
}


int
strwcasecmp(const char *str, const char *pat)
{
    switch (*pat)
    {
        case 0:
            return !(*str);

        case '*':
            return ((strwcasecmp(str, pat + 1) || 
                    (*str && strwcasecmp(str + 1, pat))));
                
        case '?':
            return (*str && strwcasecmp(str + 1, pat + 1));

        default:
            return ((tolower(*str) == tolower(*pat)) && 
                    (strwcasecmp(str + 1, pat + 1)));
    }

    return 0;
}
