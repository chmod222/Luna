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

#ifndef MM_H
#define MM_H

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

struct mm_entry
{
    void *mm_ptr;
    size_t mm_size;
};

struct mm_state
{
    struct mm_entry *mm_entries;
    size_t mm_cap;
    size_t mm_nentries;
    size_t mm_allocs;
    size_t mm_frees;
};


extern struct mm_state mm_state;

int mm_init(size_t);
void mm_destroy();

void *mm_malloc(size_t);
void *mm_realloc(void *, size_t);
void mm_free(void *);

void *mm_lalloc(void *, void *, size_t, size_t);

size_t mm_inuse();

#endif
