#ifndef ELF_H
#define ELF_H

#include <stddef.h>
#include <stdint.h>

#include "buffer.h"

typedef uint32_t Elf64_Word;
typedef uint16_t Elf64_Half;
typedef uint64_t Elf64_Off, Elf64_Addr, Elf64_Xword;

//////////////////////////////////////////////////////////////////////////////

static const size_t EI_NIDENT = 16;

struct Elf64_Ehdr
{
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
};

const unsigned char E_IDENT_TEMPLATE[] = 
{
/* [ELF----------------][bits][endian][version-][filler--------------------------------------]      */
    0x7F,0x45,0x4C,0x46, 0x02,  0x01,    0x01,   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

//////////////////////////////////////////////////////////////////////////////

const Elf64_Addr  DEFAULT_VIRTUAL_ADDR = 0x400000;
const Elf64_Xword DEFAULT_ALIGNMENT    = 0x1000;

enum P_TYPE
{
    PT_NULL = 0x0,
    PT_LOAD = 0x1,
};

enum P_FLAGS
{
    PF_EXECUTE = 0b001,
    PF_WRITE   = 0b010,
    PF_READ    = 0b100,
};

struct Elf64_Phdr 
{
	Elf64_Word	p_type;
	Elf64_Word	p_flags;
	Elf64_Off	p_offset;
	Elf64_Addr	p_vaddr;
	Elf64_Addr	p_paddr;
	Elf64_Xword	p_filesz;
	Elf64_Xword	p_memsz;
	Elf64_Xword	p_align;
};

//////////////////////////////////////////////////////////////////////////////

enum SH_TYPE
{
    SHT_NULL     = 0x0,
    SHT_PROGBITS = 0x1,
    SHT_STRTAB   = 0x3,
};

enum SH_FLAGS
{
    SHF_WRITE     = 0x1,
    SHF_ALLOC     = 0x2,
    SHF_EXECINSTR = 0x4,
};

struct Elf64_Shdr 
{
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
};

//////////////////////////////////////////////////////////////////////////////

struct Section
{
    bool        needs_phdr;
    bool        needs_shdr;
    Elf64_Word  ptype;      // type of program header
    Elf64_Word  pflags;     // flags for program header
    Elf64_Word  stype;      // type of section header
    Elf64_Xword sflags;     // flags for section header
    Elf64_Word  sname;      // index of section name in .shstrtab
    Elf64_Xword salign;     // required alignment

    const char* name;       // section name

    Buffer      buffer;     // section body

// private
    uint64_t    offset;     // offset in bytes from file beginning
    uint64_t    descriptor; // index of section in Binary
};

struct Binary
{
    Elf64_Ehdr  ehdr;          // ELF-header

    Elf64_Phdr* phdrs;         // program headers
    size_t      phdrs_num;     //
    uint64_t    phdrs_offset;  //

    Elf64_Shdr* shdrs;         // section headers
    size_t      shdrs_num;     //
    uint64_t    shdrs_offset;  //

    Section*    sections;
    size_t      sections_num;
    size_t      sections_cap;

    size_t      shstrtab_index; // index of section-header-string table

    uint64_t    occupied;
    uint64_t    hdrs_occupied;
};

//////////////////////////////////////////////////////////////////////////////

int binary_reserve_section(Binary* bin, Section* sect);
int binary_store_section(Binary* bin, Section sect);

int binary_generate_shstrtab(Binary* bin);

void binary_reserve_hdrs(Binary* bin);

int binary_arrange_sections(Binary* bin);

int binary_generate_phdrs(Binary* bin);
int binary_generate_shdrs(Binary* bin);
int binary_generate_ehdr(Binary* bin);

int binary_write(FILE* stream, Binary* bin);

void binary_dtor(Binary* bin);

void dump_ehdr(const Elf64_Ehdr* hdr);
void dump_phdr(const Elf64_Phdr* hdr);
void dump_shdr(const Elf64_Shdr* hdr);
void dump_section(const Section* sect);
void dump_binary(const Binary* bin);

#endif // ELF_H
