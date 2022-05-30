#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "elf_wrap.h"
#include "../../include/logs/logs.h"

enum binary_err
{
    BINARY_NOERR       = 0,
    BINARY_BAD_ALLOC   = 1,
    BINARY_WRITE_ERROR = 2,
};

///////////////////////////////////////////////////////////////////////////////

// ------------------------------ Overview ------------------------------
// These functions provide interface for creating ELF executable file.
// 
// 1. Reserve place for sections. Sections will get descriptors.
// 2. Fill sections with binary code. Initialize needed fields.
//    Descriptors can be saved to use as a reference to section
//    in Binary structure.
// 3. Store sections in Binary.
// 4. Generate shstrtab for sections.
// 5. Reserve place for headers.
// 6. Arrange sections.
// 7. Generate headers.

///////////////////////////////////////////////////////////////////////////////

static void section_dtor(Section* sect)
{
    assert(sect);

    assert(sect->buffer.size == sect->shdr.sh_size);
    assert(sect->offset == sect->shdr.sh_offset);

    buffer_dtor(&sect->buffer);

    *sect = {};
}

///////////////////////////////////////////////////////////////////////////////

static int binary_sections_resize(Binary* bin, size_t new_cap)
{
    assert(bin);

    if(new_cap < 4)
        new_cap = 4;

    if(new_cap < bin->sections_cap)
        return BINARY_NOERR;

    Section* ptr = (Section*) realloc(bin->sections, new_cap * sizeof(Section));
    ASSERT_RET$(ptr, BINARY_BAD_ALLOC);

    bin->sections     = ptr;
    bin->sections_cap = new_cap;

    return BINARY_NOERR;
}

// Reserves place for Section in Binary and sets section descriptor.
// Expect: +sect.name
// Modify: +sect.descriptor
int binary_reserve_section(Binary* bin, Section* sect)
{
    assert(bin);

    if(bin->sections_cap == bin->sections_num)
        ASSERT_RET$(!binary_sections_resize(bin, bin->sections_cap * 2), BINARY_BAD_ALLOC);

    sect->descriptor = bin->sections_num;
    bin->sections[bin->sections_num] = *sect;
    bin->sections_num++;

    return 0;
}

// Updates previously reserved Section in Binary.
// Expect: +sect -sect.offset
// Modify: +sect -sect.offset
int binary_store_section(Binary* bin, Section sect)
{
    assert(bin);
    assert(sect.descriptor < bin->sections_num);

    Section* ptr = &bin->sections[sect.descriptor];
    assert(ptr->descriptor == sect.descriptor && "Section has invalid descriptor");

    *ptr = sect;

    return 0;
}

// Evaluates amount of space for ehdr, phdrs and shdrs.
// Expect: +bin.sections -bin.sections.offset
// Modify: +bin.occupied +bin.shdrs_num
void binary_reserve_hdrs(Binary* bin)
{
    assert(bin);

    bin->hdrs_occupied = sizeof(Elf64_Ehdr) + bin->sections_num * sizeof(Elf64_Shdr);
    bin->occupied = bin->hdrs_occupied;
}

// Arranges sections' offsets in file.
// Expect: +bin.sections -bin.sections.offset
// Modify: +bin.occupied +bin.sections.offset
int binary_arrange_sections(Binary* bin)
{
    assert(bin);

    uint64_t alignment = DEFAULT_ALIGNMENT;

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];
        if(sect->buffer.size == 0)
            continue;

        sect->offset = alignment * ((bin->occupied + alignment - 1) / alignment);
        sect->shdr.sh_offset = sect->offset;
        sect->shdr.sh_size   = sect->buffer.size;
        bin->occupied = sect->offset + sect->buffer.size;
    }

    return 0;
}

// Generates section headers.
// Expect: +bin.sections
// Modify: +bin.shdrs
int binary_generate_shdrs(Binary* bin)
{
    assert(bin);

    bin->shdrs = (Elf64_Shdr*) calloc(bin->sections_num, sizeof(Elf64_Shdr));

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];

        bin->shdrs[iter] = sect->shdr;
    }

    return 0;
}

// Generates ELF header.
// Expect: +bin.sections +bin.shdrs
// Modify: +bin.ehdr
int binary_generate_ehdr(Binary* bin)
{
    assert(bin);

    Elf64_Ehdr hdr = {};

    memcpy(hdr.e_ident, E_IDENT_TEMPLATE, sizeof(E_IDENT_TEMPLATE));

    hdr.e_type        = 0x01;
    hdr.e_machine     = 0x3E;
    hdr.e_version     = 0x1;

    hdr.e_entry       = 0x0;

    hdr.e_phoff       = 0x0;
    hdr.e_shoff       = 0x40;
    bin->shdrs_offset = hdr.e_shoff;

    assert(bin->sections_num   < UINT16_MAX);
    assert(bin->shstrtab_index < UINT16_MAX);
    
    hdr.e_flags       = 0x0;
    hdr.e_ehsize      = (uint16_t) sizeof(Elf64_Ehdr);
    hdr.e_phentsize   = 0x0;
    hdr.e_phnum       = 0;
    hdr.e_shentsize   = (uint16_t) sizeof(Elf64_Shdr);
    hdr.e_shnum       = (uint16_t) bin->sections_num;
    hdr.e_shstrndx    = (uint16_t) bin->shstrtab_index;

    bin->ehdr = hdr;

    return 0;
}

// Generates section-header-strings table and stores in Binary.
// Sets shstrtab_descriptor for ehdr creation.
// Sets sections' sname for shdrs creation.
// Expect: +bin.sections.name
// Modify: +bin.sections.sname +bin.shstrtab_index
int binary_generate_shstrtab(Binary* bin)
{
    assert(bin);

    Section shstrtab = {(Elf64_Shdr)
                        {
                            .sh_type = SHT_STRTAB,
                            .sh_flags = 0x0,
                            .sh_addralign = 1
                        },
                        .name = ".shstrtab"
                       };
    
    // Reserving section before for-loop results in creating string for .shstrtab itself.
    PASS$(!binary_reserve_section(bin, &shstrtab), return BINARY_BAD_ALLOC; );

    // We need to edit shstrtab directly in bin.sections, because using local Section 
    // and binary_store_section() will overwrite sname.
    Section* shstrtab_ptr = &bin->sections[shstrtab.descriptor];

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];

        assert(sect->name && "Section name wasn't set");
        sect->shdr.sh_name = (uint32_t) shstrtab_ptr->buffer.size;
        size_t len = strlen(sect->name);
        ASSERT_RET$(!buffer_append_arr(&shstrtab_ptr->buffer, sect->name, len + 1), BINARY_BAD_ALLOC); // +1 to append null-terminated string
    }

    bin->shstrtab_index = bin->sections_num - 1;

    return 0;
}

// Writes headers, sections to file according to Binary setup.
// Expect: +bin.sections +bin.shdrs +bin.ehdr
// Modify:
int binary_write(FILE* stream, Binary* bin)
{
    assert(stream && bin);
    assert(bin->hdrs_occupied >= sizeof(Elf64_Ehdr) + bin->sections_num * sizeof(Elf64_Shdr));

    uint8_t* buffer = (uint8_t*) calloc(bin->occupied, sizeof(uint8_t));

    memcpy(buffer, &bin->ehdr, sizeof(Elf64_Ehdr));
    memcpy(buffer + bin->shdrs_offset, bin->shdrs, bin->sections_num * sizeof(Elf64_Shdr));

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];
        
        if(sect->buffer.buf)
            memcpy(buffer + sect->offset, sect->buffer.buf, sect->buffer.size);
    }

    uint64_t written = fwrite(buffer, sizeof(uint8_t), bin->occupied, stream);
    ASSERT_RET$(written == bin->occupied, BINARY_WRITE_ERROR);

    free(buffer);

    return 0;
}

void binary_dtor(Binary* bin)
{
    assert(bin);

    free(bin->shdrs);

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        section_dtor(&bin->sections[iter]);
    }

    free(bin->sections);

    *bin = {};
}

///////////////////////////////////////////////////////////////////////////////

int binary_generate_rela(Section* relatbl, Symtable* table, Relocations* relocs)
{
    assert(relatbl && table && relocs);

    for(size_t iter = 0; iter < relocs->buffer_sz; iter++)
    {
        Reloc* reloc = &relocs->buffer[iter];
        Symbol* sym  = &table->buffer[reloc->src_nametable_index];

        Elf64_Xword rinfo = 0;
        if(sym->type == SYMBOL_TYPE_FUNCTION)
            rinfo = (reloc->src_nametable_index << 32) + R_X86_64_PLT32;
        else
            rinfo = (reloc->src_nametable_index << 32) + R_X86_64_PC32;

        Elf64_Rela rela = {.r_offset = reloc->dst_offset,
                           .r_info   = rinfo,
                           .r_addend = reloc->dst_init_val
                          };
        
        ASSERT_RET$(!buffer_append_arr(&relatbl->buffer, &rela, sizeof(Elf64_Rela)), BINARY_BAD_ALLOC);
    }

    return 0;
}

int binary_generate_strtab(Section* strtab, Symtable* table)
{
    assert(strtab && table);

    for(size_t iter = 0; iter < table->buffer_sz; iter++)
    {        
        Symbol* sym = &table->buffer[iter];

        assert(sym->id && "No symbol name");
        sym->s_name = strtab->buffer.size;
        size_t len = strlen(sym->id);
        ASSERT_RET$(!buffer_append_arr(&strtab->buffer, sym->id, len + 1), BINARY_BAD_ALLOC); // +1 to append null-terminated string
    }

    return 0;
}

int binary_generate_symtab(Section* symtab, Symtable* table)
{
    assert(symtab && table);

    for(size_t iter = 0; iter < table->buffer_sz; iter++)
    {
        Symbol* sym = &table->buffer[iter];

        Elf64_Sym elf_sym = {.st_name  = (Elf64_Word) sym->s_name,
                             .st_info  = (STB_GLOBAL << 4) + STT_FUNC,
                             .st_other = STV_DEFAULT,
                             .st_shndx = (Elf64_Half) sym->section_descriptor,
                             .st_value = sym->offset,
                             .st_size  = sym->s_size
                            };

        ASSERT_RET$(!buffer_append_arr(&symtab->buffer, &elf_sym, sizeof(Elf64_Sym)), BINARY_BAD_ALLOC);
    }

    return 0;
}
