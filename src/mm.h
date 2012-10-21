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

#endif /* define MM_H */
