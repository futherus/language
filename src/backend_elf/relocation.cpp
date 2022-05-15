#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "relocation.h"
#include "elf.h"

///////////////////////////////////////////////////////////////////////////////

static int relocations_resize(Relocations* rel, size_t new_cap)
{
    assert(rel);

    assert(new_cap > rel->buffer_cap);

    Reloc* ptr = (Reloc*) realloc(rel->buffer, new_cap * sizeof(Reloc));
    assert(ptr);

    rel->buffer     = ptr;
    rel->buffer_cap = new_cap;

    return 0;
}

int relocations_ctor(Relocations* rel)
{
    assert(rel);
    rel = {};

    relocations_resize(rel, 256);

    return 0;
}

void relocations_dtor(Relocations* rel)
{
    assert(rel);

    free(rel->buffer);

    rel = {};
}

int relocations_insert(Relocations* rel, Reloc reloc)
{
    assert(rel);

    if(rel->buffer_cap <= rel->buffer_sz)
        relocations_resize(rel, rel->buffer_cap * 2);

    rel->buffer[rel->buffer_sz] = reloc;

    return 0;
}

int relocations_resolve(Relocations* rel, Symtable* tbl, Binary* bin)
{
    assert(rel && tbl);

    for(size_t iter = 0; iter < rel->buffer_sz; iter++)
    {
        const Reloc*   reloc    = &rel->buffer[iter];

        Section*       dst_sect = &bin->sections[reloc->dst_section_descriptor];

        const Symbol*  src_sym  = &tbl->buffer[reloc->src_nametable_index];
        const Section* src_sect = &bin->sections[src_sym->section_descriptor];

        assert(reloc->dst_addr_size == sizeof(int32_t) && "Invalid displacement size");

        int32_t disp = (int32_t) ( 
                       (src_sect->offset + src_sym->offset) - 
                       (dst_sect->offset + reloc->dst_offset + reloc->dst_addr_size)
                    );

        memcpy(&dst_sect->buffer[reloc->dst_offset], &disp, reloc->dst_addr_size);
    }

    return 0;
}
