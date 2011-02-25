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
 *  Simple linked list implementation (linked_list.c)
 *  ---
 *  Construct, free and maintain a singly linked list
 *
 *  Created: 25.02.2011 16:22:07
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "linked_list.h"


int
list_init(linked_list **list)
{
    *list = malloc(sizeof(**list));
    if (*list)
    {
        (*list)->root = NULL;
        (*list)->length = 0;

        return 0;
    }

    return 1;
}


list_node *
list_get_root(linked_list *list)
{
    if (list)
        return list->root;
    else
        return NULL;
}


list_node *
list_push_front(linked_list *list, void *data)
{
    list_node *tmp = NULL;

    if (list == NULL)
        return NULL;

    tmp = malloc(sizeof(list_node));
    if (tmp)
    {
        list_node *root = list_get_root(list);

        memset(tmp, 0, sizeof(*tmp));
        tmp->data = data;

        if (root)
        {
            list->root = tmp;
            tmp->next = root;
        }
        else
        {
            list->root = tmp;
        }
    }

    return tmp;
}


list_node *
list_insert_before(linked_list *list, list_node *pos, void *data)
{
    list_node *tmp = NULL;
    list_node *pre = NULL;

    if ((list == NULL) || (pos == NULL))
        return NULL;

    if ((list_get_root(list) == NULL) || (pos == list_get_root(list)))
        return list_push_front(list, data);

    pre = list_get_root(list);
    while (pre->next != pos)
    {
        pre = pre->next;

        if (!pre)
            return NULL;
    }

    tmp = malloc(sizeof(list_node));
    if (tmp)
    {
        memset(tmp, 0, sizeof(*tmp));

        tmp->data = data;
        tmp->next = pos;
        pre->next = tmp;
    }

    return tmp;
}


list_node *
list_insert_after(linked_list *list, list_node *pos, void *data)
{
    if ((list == NULL) || (pos == NULL))
        return NULL;

    if (pos->next == NULL)
        return list_push_back(list, data);
    else
        return list_insert_before(list, pos->next, data);
}


list_node *
list_push_back(linked_list *list, void *data)
{
    list_node *tmp = NULL;

    if (list == NULL)
        return NULL;

    tmp = malloc(sizeof(list_node));
    if (tmp)
    {
        list_node *root = list_get_root(list);

        memset(tmp, 0, sizeof(*tmp));
        tmp->data = data;

        if (root)
        {
            list_node *current = root;

            while (current->next != NULL)
                current = current->next;

            current->next = tmp;
        }
        else
        {
            list->root = tmp;
        }
    }

    return tmp;
}


list_node *
list_find(linked_list *list, void *data, int (*cmp)(void *, void *))
{
    list_node *cur = NULL;

    for (cur = list->root; cur != NULL; cur = cur->next)
        if (!cmp(data, cur->data))
            return cur->data;

    return NULL;
}


void
list_delete(linked_list *list, void *plugin, void (*f)(void *))
{
    if (list == NULL)
        return;

    list_node *old = NULL;
    list_node *current = list->root;

    while (current != NULL)
    {
        if (current->data == plugin)
        {
            if (old == NULL)
                list->root = current->next;
            else
                old->next = current->next;
            
            f(current->data);
            free(current);

            list->length--;

            return;
        }

        old = current;
        current = current->next;
    }

    return;
}


void
list_map(linked_list *list, void (*f)(void *))
{
    if (list == NULL)
        return;

    list_node *current = list->root;

    while (current != NULL)
    {
        f(current->data);

        current = current->next;
    }

    return;
}


void
list_destroy(linked_list *list, void (*free_content)(void *))
{
    list_node *current = NULL;

    if (list == NULL) 
        return;

    current = list->root;

    while (current != NULL)
    {
        list_node *next = current->next;

        if (free_content != NULL)
            free_content(current->data);

        free(current);
        current = next;
    }

    free(list);

    return;
}
