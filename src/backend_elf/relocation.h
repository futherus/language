#ifndef RELOCATION_H
#define RELOCATION_H

#include <stddef.h>
#include <stdint.h>

#include "symtable.h"
#include "encode.h"
#include "elf.h"

struct Reloc
{
    uint64_t    dst_section_descriptor;
    uint64_t    dst_offset;
    int32_t     dst_init_val;

    uint64_t src_nametable_index;
};

struct Relocations
{
    Reloc* buffer;
    size_t buffer_sz;
    size_t buffer_cap;
};

int  relocations_ctor(Relocations* rel);
void relocations_dtor(Relocations* rel);

int  relocations_insert (Relocations* rel, Reloc reloc);
int  relocations_resolve(Relocations* rel, Symtable* tbl, Binary* bin);

#endif // RELOCATION_H
