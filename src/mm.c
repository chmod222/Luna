#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mm.h"
#include "luna.h"

struct mm_state mm_state;


int mm_init(size_t initcount)
{
    size_t initsize = initcount * sizeof(struct mm_entry);

    memset(&mm_state, 0, sizeof(mm_state));

    if ((mm_state.mm_entries = malloc(initsize)) == NULL)
        return 1;

    memset(mm_state.mm_entries, 0, initsize);

    mm_state.mm_cap = initcount;
    mm_state.mm_nentries = 0;

#ifdef MMDEBUG
    printf("mm: new allocation table of size %lu (%lu entries)\n", initsize, initcount);
#endif

    return 0;
}

void mm_destroy()
{
    int i;

#ifdef MMDEBUG
    printf("mm: cleaning up %lu blocks...\n", mm_state.mm_nentries);
#endif

    for (i = 0; i < mm_state.mm_nentries; ++i)
    {
        free(mm_state.mm_entries[i].mm_ptr);
        mm_state.mm_frees++;
    }

    free(mm_state.mm_entries);
}

void *mm_malloc(size_t n)
{
    void *ptr = NULL;
    struct mm_entry *next = NULL;

    if ((ptr = malloc(n)) == NULL)
        return NULL;

    if (!(mm_state.mm_nentries < mm_state.mm_cap))
    {
        /* Allocate some more */
        size_t size = mm_state.mm_cap * 2 * sizeof(struct mm_entry);

        if ((mm_state.mm_entries = realloc(mm_state.mm_entries, size)) == NULL)
        {
            free(ptr);
            return NULL;
        }

        mm_state.mm_cap *= 2;
    }

    next = &mm_state.mm_entries[mm_state.mm_nentries++];

    next->mm_ptr = ptr;
    next->mm_size = n;
    mm_state.mm_allocs++;

#ifdef MMDEBUG
    printf("mm: allocated %lu bytes at %p (%lu entries left)\n",
            n, ptr, mm_state.mm_cap - (mm_state.mm_nentries));
#endif

    return ptr;
}

void *mm_realloc(void *ptr, size_t newsize)
{
    int i;

    if (ptr == NULL)
        return mm_malloc(newsize);

    if (newsize == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    for (i = 0; i < mm_state.mm_nentries; ++i)
    {
        if (mm_state.mm_entries[i].mm_ptr == ptr)
        {
            struct mm_entry *e = &(mm_state.mm_entries[i]);
            void *newd = realloc(e->mm_ptr, newsize);

#ifdef MMDEBUG
            printf("mm: realloc block %p to %lu bytes.. ", e->mm_ptr, newsize);
#endif

            if (newd == NULL)
            {
#ifdef MMDEBUG
                printf("failed\n");
#endif
                return ptr;
            }
            else
            {
#ifdef MMDEBUG
                printf("success, new block at %p\n", newd);
#endif

                e->mm_ptr = newd;
                e->mm_size = newsize;

                return newd;
            }
        }
    }

    return NULL;
}

void mm_free(void *ptr)
{
    int i;

    if (ptr == NULL)
        return;

    for (i = 0; i < mm_state.mm_nentries; ++i)
    {
        if (mm_state.mm_entries[i].mm_ptr == ptr)
        {
            int j;

#ifdef MMDEBUG
            printf("mm: freed %lu bytes at %p (%lu left)\n",
                    mm_state.mm_entries[i].mm_size,
                    mm_state.mm_entries[i].mm_ptr,
                    mm_state.mm_cap - (mm_state.mm_nentries - 1));
#endif

            free(mm_state.mm_entries[i].mm_ptr);
            mm_state.mm_entries[i].mm_ptr = NULL;
            mm_state.mm_frees++;

            for (j = i; j < mm_state.mm_nentries; ++j)
                memcpy(&(mm_state.mm_entries[j]),
                       &(mm_state.mm_entries[j + 1]), sizeof(struct mm_entry));

            mm_state.mm_nentries--;

            break;
        }
    }

    return;
}

void *mm_lalloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (nsize == 0)
    {
        mm_free(ptr);
        return NULL;
    }
    else
    {
        return mm_realloc(ptr, nsize);
    }
}

size_t mm_inuse()
{
    int i;
    size_t inuse = mm_state.mm_cap * sizeof(struct mm_entry);

    for (i = 0; i < mm_state.mm_nentries; ++i)
        inuse += mm_state.mm_entries[i].mm_size;

    return inuse;
}
