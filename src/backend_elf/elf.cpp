#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "elf.h"

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
// 8. Resolve relocations. Descriptors provide references.

static void section_dtor(Section* sect)
{
    assert(sect);

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
        return 0;

    Section* ptr = (Section*) realloc(bin->sections, new_cap * sizeof(Section));
    assert(ptr);

    bin->sections     = ptr;
    bin->sections_cap = new_cap;

    return 0;
}

// Reserves place for Section in Binary and sets section descriptor
// Expect: +sect.name
// Modify: +sect.descriptor
int binary_reserve_section(Binary* bin, Section* sect)
{
    assert(bin);

    if(bin->sections_cap == bin->sections_num)
    {
        binary_sections_resize(bin, bin->sections_cap * 2);
    }

    sect->descriptor = bin->sections_num;
    bin->sections[bin->sections_num] = *sect;
    bin->sections_num++;

    return 0;
}

// Updates previously reserved Section in Binary
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

// Generates section-header-strings table and stores in Binary
// Sets shstrtab_descriptor for ehdr creation
// Sets sections' sname for shdrs creation
// Expect: +bin.sections.name +bin.sections.needs_shdr
// Modify: +bin.sections.sname +bin.shstrtab_descriptor
int binary_generate_shstrtab(Binary* bin)
{
    assert(bin);

    Section shstrtab = {.needs_phdr = false,
                        .needs_shdr = true,
                        .ptype  = 0x0,
                        .pflags = 0x0,
                        .stype  = SHT_STRTAB,
                        .sflags = 0x0,
                        .salign = 0x1,
                        .name = ".shstrtab"
                       };
    
    // Reserving section before for-loop results in creating string for .shstrtab itself
    binary_reserve_section(bin, &shstrtab);

    // We need to edit shstrtab directly in bin.sections, because using local Section 
    // and binary_store_section() will overwrite sname
    Section* shstrtab_ptr = &bin->sections[shstrtab.descriptor];
    uint64_t shdr_num = 0;

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];
        if(!sect->needs_shdr)
            continue;

        shdr_num++;
        assert(sect->name && "Section name wasn't set");
        sect->sname = (uint32_t) shstrtab_ptr->buffer.size;
        size_t len = strlen(sect->name);
        buffer_append_arr(&shstrtab_ptr->buffer, sect->name, len + 1); // +1 to append null-terminated string
    }

    shdr_num--; // shstrtab section header is the last

    bin->shstrtab_index = shdr_num;

    return 0;
}

// Evaluates amount of space for ehdr, phdrs and shdrs
// Expect: +bin.sections -bin.sections.offset
// Modify: +bin.occupied +bin.phdrs_num +bin.shdrs_num
void binary_reserve_hdrs(Binary* bin)
{
    assert(bin);

    bin->phdrs_num = 1; // +1 because of PT_LOAD phdr for loading elf, program and section headers themselves
    bin->shdrs_num = 0;

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];

        bin->phdrs_num += sect->needs_phdr;
        bin->shdrs_num += sect->needs_shdr;
    }

    // Actual phdrs_num can be smaller because different sections can be concatenated to the same segment
    bin->hdrs_occupied = sizeof(Elf64_Ehdr) + bin->phdrs_num * sizeof(Elf64_Phdr) + bin->shdrs_num * sizeof(Elf64_Shdr);
    bin->occupied = bin->hdrs_occupied;
}

// Arranges sections offsets in file
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

        bin->occupied = sect->offset + sect->buffer.size;
    }

    return 0;
}

// Generates program headers for sections and for headers
// themselves
// Expect: +bin.sections
// Modify: +bin.phdrs
int binary_generate_phdrs(Binary* bin)
{
    assert(bin);

    Elf64_Phdr init_hdr = {.p_type   = PT_LOAD,
                           .p_flags  = PF_READ,
                           .p_offset = 0x0,
                           .p_vaddr  = DEFAULT_VIRTUAL_ADDR,
                           .p_paddr  = DEFAULT_VIRTUAL_ADDR,
                           .p_filesz = bin->hdrs_occupied,
                           .p_memsz  = bin->hdrs_occupied,
                           .p_align  = DEFAULT_ALIGNMENT,
                          };

    bin->phdrs = (Elf64_Phdr*) calloc(bin->phdrs_num, sizeof(Elf64_Phdr));
    assert(bin->phdrs);

    bin->phdrs[0] = init_hdr;

    size_t phdr_iter = 1; // +1 because of init_hdr
    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];

        if(!sect->needs_phdr)
            continue;

        Elf64_Phdr phdr = {.p_type   = sect->ptype,
                           .p_flags  = sect->pflags,
                           .p_offset = sect->offset,
                           .p_vaddr  = DEFAULT_VIRTUAL_ADDR + sect->offset,
                           .p_paddr  = DEFAULT_VIRTUAL_ADDR + sect->offset,
                           .p_filesz = sect->buffer.size,
                           .p_memsz  = sect->buffer.size,
                           .p_align  = DEFAULT_ALIGNMENT,
                          };        
        
        bin->phdrs[phdr_iter] = phdr;
        phdr_iter++;
    }

    return 0;
}

// Generates section headers
// Expect: +bin.sections
// Modify: +bin.shdrs
int binary_generate_shdrs(Binary* bin)
{
    assert(bin);

    bin->shdrs = (Elf64_Shdr*) calloc(bin->shdrs_num, sizeof(Elf64_Shdr));
    assert(bin->shdrs);

    size_t shdr_iter = 0;
    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];

        if(!sect->needs_shdr)
            continue;
        
        Elf64_Shdr shdr = {.sh_name      = sect->sname,
                           .sh_type      = sect->stype,
                           .sh_flags     = sect->sflags,
                           .sh_addr      = DEFAULT_VIRTUAL_ADDR + sect->offset,
                           .sh_offset    = sect->offset,
                           .sh_size      = sect->buffer.size,
                           .sh_link      = 0x0,
                           .sh_info      = 0x0,
                           .sh_addralign = sect->salign,
                           .sh_entsize   = 0x0
                          };
        
        if(sect->needs_phdr)
            shdr.sh_addr = DEFAULT_VIRTUAL_ADDR + sect->offset;
        else
            shdr.sh_addr = 0;

        bin->shdrs[shdr_iter] = shdr;
        shdr_iter++;
    }

    return 0;
}

// Generates ELF header
// Expect: +bin.sections +bin.phdrs +bin.shdrs
// Modify: +bin.ehdr
int binary_generate_ehdr(Binary* bin)
{
    assert(bin);

    Elf64_Ehdr hdr = {};

    memcpy(hdr.e_ident, E_IDENT_TEMPLATE, sizeof(E_IDENT_TEMPLATE));

    hdr.e_type        = 0x02;
    hdr.e_machine     = 0x3E;
    hdr.e_version     = 0x1;

    hdr.e_entry       = 0x401000;

    hdr.e_phoff       = 0x40;
    bin->phdrs_offset = hdr.e_phoff;
    hdr.e_shoff       = hdr.e_phoff + bin->phdrs_num * sizeof(Elf64_Phdr);
    bin->shdrs_offset = hdr.e_shoff;

    assert(bin->phdrs_num      < UINT16_MAX);
    assert(bin->shdrs_num      < UINT16_MAX);
    assert(bin->shstrtab_index < UINT16_MAX);
    
    hdr.e_flags       = 0x0;
    hdr.e_ehsize      = (uint16_t) sizeof(Elf64_Ehdr);
    hdr.e_phentsize   = (uint16_t) sizeof(Elf64_Phdr);
    hdr.e_phnum       = (uint16_t) bin->phdrs_num;
    hdr.e_shentsize   = (uint16_t) sizeof(Elf64_Shdr);
    hdr.e_shnum       = (uint16_t) bin->shdrs_num;
    hdr.e_shstrndx    = (uint16_t) bin->shstrtab_index;

    bin->ehdr = hdr;

    return 0;
}

// Writes headers, sections to file according to Binary setup
// Expect: +bin.sections +bin.phdrs +bin.shdrs +bin.ehdr
// Modify:
int binary_write(FILE* stream, Binary* bin)
{
    assert(stream && bin);
    assert(bin->hdrs_occupied >= sizeof(Elf64_Ehdr) +
                                 bin->phdrs_num * sizeof(Elf64_Phdr) +
                                 bin->shdrs_num * sizeof(Elf64_Shdr));

    uint8_t* buffer = (uint8_t*) calloc(bin->occupied, sizeof(uint8_t));

    memcpy(buffer, &bin->ehdr, sizeof(Elf64_Ehdr));
    memcpy(buffer + bin->phdrs_offset, bin->phdrs, bin->phdrs_num * sizeof(Elf64_Phdr));
    memcpy(buffer + bin->shdrs_offset, bin->shdrs, bin->shdrs_num * sizeof(Elf64_Shdr));

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        Section* sect = &bin->sections[iter];
        
        if(sect->buffer.buf)
            memcpy(buffer + sect->offset, sect->buffer.buf, sect->buffer.size);
    }

    fwrite(buffer, sizeof(uint8_t), bin->occupied, stream);

    free(buffer);

    return 0;
}

void binary_dtor(Binary* bin)
{
    assert(bin);

    free(bin->phdrs);
    free(bin->shdrs);

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        section_dtor(&bin->sections[iter]);
    }

    free(bin->sections);

    *bin = {};
}

///////////////////////////////////////////////////////////////////////////////

void dump_ehdr(const Elf64_Ehdr* hdr)
{
    assert(hdr);

    const uint8_t* id = (const uint8_t*) hdr;
    printf("ELF header:\n");
    printf("|%02hx %02hx %02hx %02hx|%02hx %02hx|%02hx|%02hx|%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx|\n",
        id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7], id[8], id[9], id[10], id[11], id[12], id[13], id[14], id[15]);
    printf("|%02hx %02hx|%02hx %02hx|%02hx %02hx %02hx %02hx|%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx|\n",
        id[16], id[17], id[18], id[19], id[20], id[21], id[22], id[23], id[24], id[25], id[26], id[27], id[28], id[29], id[30], id[31]);
    printf("|%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx|%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx|\n",
        id[32], id[33], id[34], id[35], id[36], id[37], id[38], id[39], id[40], id[41], id[42], id[43], id[44], id[45], id[46], id[47]);
    printf("|%02hx %02hx %02hx %02hx|%02hx %02hx|%02hx %02hx|%02hx %02hx|%02hx %02hx|%02hx %02hx|%02hx %02hx|\n",
        id[48], id[49], id[50], id[51], id[52], id[53], id[54], id[55], id[56], id[57], id[58], id[59], id[60], id[61], id[62], id[63]);
}

void dump_phdr(const Elf64_Phdr* hdr)
{
    assert(hdr);

    printf("p_type:   0x%04x\n"
           "p_flags:  0x%04x\n"
           "p_offset: 0x%08lx\n"
           "p_vaddr:  0x%08lx\n"
           "p_paddr:  0x%08lx\n"
           "p_filesz: 0x%08lx\n"
           "p_memsz:  0x%08lx\n"
           "p_align:  0x%08lx\n",
           hdr->p_type, hdr->p_flags, hdr->p_offset, hdr->p_vaddr, hdr->p_paddr, hdr->p_filesz, hdr->p_memsz, hdr->p_align);
}

void dump_shdr(const Elf64_Shdr* hdr)
{
    assert(hdr);

    printf("sh_name:      0x%04x\n"
           "sh_type:      0x%04x\n"
           "sh_flags:     0x%08lx\n"
           "sh_addr:      0x%08lx\n"
           "sh_offset:    0x%08lx\n"
           "sh_size:      0x%08lx\n"
           "sh_link:      0x%04x\n"
           "sh_info:      0x%04x\n"
           "sh_addralign: 0x%08lx\n"
           "sh_entsize:   0x%08lx\n",
           hdr->sh_name, hdr->sh_type, hdr->sh_flags, hdr->sh_addr, hdr->sh_offset, hdr->sh_size, hdr->sh_link, hdr->sh_info, hdr->sh_addralign, hdr->sh_entsize);
}

void dump_section(const Section* sect)
{
    assert(sect);

    printf("Name:       %s\n", sect->name);

    printf("Needs phdr: %d\n", sect->needs_phdr);
    printf("Needs shdr: %d\n", sect->needs_phdr);
    printf("p_type:     0x%04x\n",  sect->ptype);
    printf("p_flags:    0x%04x\n",  sect->pflags);
    printf("s_type:     0x%04x\n",  sect->stype);
    printf("s_flags:    0x%08lx\n", sect->sflags);
    printf("s_align:    0x%08lx\n", sect->salign);

    printf("Buffer:    [%p]\n", sect->buffer.buf);
    printf("Buffer_sz:  %08lu\n", sect->buffer.size);
    printf("Buffer_cap: %08lu\n", sect->buffer.cap);

    printf("Relocations:    XXX\n");
    printf("Relocations_sz: XXX\n");

    printf("Offset:     0x%08lx\n", sect->offset);
    printf("Descriptor: 0x%08lx\n", sect->descriptor);
}

void dump_binary(const Binary* bin)
{
    assert(bin);

    printf("Occupied by headers: %lu\n",   bin->hdrs_occupied);
    printf("Occupied total:      %lu\n\n", bin->occupied);

    dump_ehdr(&bin->ehdr);

    printf("\n%lu program header(s), starting at offset 0x%lx\n\n", bin->phdrs_num, bin->phdrs_offset);

    for(size_t iter = 0; iter < bin->phdrs_num; iter++)
    {
        dump_phdr(&bin->phdrs[iter]);
        printf("\n");
    }

    printf("\n%lu section header(s), starting at offset 0x%lx\n\n", bin->shdrs_num, bin->shdrs_offset);
    
    for(size_t iter = 0; iter < bin->shdrs_num; iter++)
    {
        dump_shdr(&bin->shdrs[iter]);
        printf("\n");
    }

    printf("%lu section(s):\n\n", bin->sections_num);

    for(size_t iter = 0; iter < bin->sections_num; iter++)
    {
        dump_section(&bin->sections[iter]);
        printf("\n");
    }
}

///////////////////////////////////////////////////////////////////////////////

/*

int main()
{
    Binary bin = {};

    struct stat st = {};
    stat("text", &st);
    size_t infile_sz = st.st_size;

    uint8_t* inbuf = (uint8_t*) calloc(infile_sz, sizeof(uint8_t));

    FILE* infile = fopen("text", "rb");
    fread(inbuf, sizeof(uint8_t), infile_sz, infile);

    size_t buffer_sz = 30;
    uint8_t* buffer = (uint8_t*) calloc(30, sizeof(uint8_t));

    memset(buffer, '#', buffer_sz);
    memcpy(buffer, "I am section!", 13);
    
    Section text = {.needs_phdr = true,
                    .needs_shdr = true,
                    .ptype  = PT_LOAD,
                    .pflags = PF_EXECUTE | PF_READ,
                    .stype  = SHT_PROGBITS,
                    .sflags = SHF_EXECINSTR | SHF_ALLOC,
                    .salign = 16,
                    .name = ".text"
                   };

    Section sect = {.needs_phdr = true,
                    .needs_shdr = true,
                    .ptype  = PT_LOAD,
                    .pflags = PF_EXECUTE | PF_READ,
                    .stype  = SHT_PROGBITS,
                    .sflags = SHF_EXECINSTR,
                    .salign = 16,
                    .name = ".mysection"
                   };

    Section null = {.needs_phdr = false,
                    .needs_shdr = true,
                    .stype  = SHT_NULL,
                    .name = ""
                   };

    binary_reserve_section(&bin, &null);
    binary_reserve_section(&bin, &text);
    binary_reserve_section(&bin, &sect);
    
    sect.buffer     = buffer;
    sect.buffer_sz  = buffer_sz;
    sect.buffer_cap = buffer_sz;

    text.buffer     = inbuf;
    text.buffer_sz  = infile_sz;
    text.buffer_cap = infile_sz;
    
    binary_store_section(&bin, text);
    binary_store_section(&bin, sect);

    binary_generate_shstrtab(&bin);

    binary_reserve_hdrs(&bin);

    binary_arrange_sections(&bin);

    binary_generate_phdrs(&bin);
    binary_generate_shdrs(&bin);
    binary_generate_ehdr(&bin);

    dump_binary(&bin);

    FILE* stream = fopen("output", "wb");

    binary_write(stream, &bin);

    fclose(stream);
    binary_dtor(&bin);

    return 0;
}

*/