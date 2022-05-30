#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "relocation.h"
#include "../../include/logs/logs.h"

enum relocations_err
{
    RELOCATIONS_NOERR     = 0,
    RELOCATIONS_BAD_ALLOC = 1,
};

///////////////////////////////////////////////////////////////////////////////

static int relocations_resize(Relocations* rel, size_t new_cap)
{
    assert(rel);

    assert(new_cap > rel->buffer_cap);

    Reloc* ptr = (Reloc*) realloc(rel->buffer, new_cap * sizeof(Reloc));
    ASSERT_RET$(ptr, RELOCATIONS_BAD_ALLOC);

    rel->buffer     = ptr;
    rel->buffer_cap = new_cap;

    return 0;
}

int relocations_ctor(Relocations* rel)
{
    assert(rel);
    *rel = {};

    PASS$(!relocations_resize(rel, 256), return RELOCATIONS_BAD_ALLOC; );

    return 0;
}

void relocations_dtor(Relocations* rel)
{
    assert(rel);

    free(rel->buffer);

    *rel = {};
}

int relocations_insert(Relocations* rel, Reloc reloc)
{
    assert(rel);

    if(rel->buffer_cap <= rel->buffer_sz)
        PASS$(!relocations_resize(rel, rel->buffer_cap * 2), return RELOCATIONS_BAD_ALLOC; );

    rel->buffer[rel->buffer_sz] = reloc;
    rel->buffer_sz++;

    return 0;
}
