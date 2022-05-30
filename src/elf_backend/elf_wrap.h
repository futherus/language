#ifndef ELF_WRAP_H
#define ELF_WRAP_H

#include <stddef.h>
#include <stdint.h>

#include "buffer.h"
#include "symtable.h"
#include "relocation.h"

typedef uint32_t Elf64_Word;
typedef uint16_t Elf64_Half;
typedef uint64_t Elf64_Off, Elf64_Addr, Elf64_Xword;
typedef int64_t  Elf64_Sxword;

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

const Elf64_Xword DEFAULT_ALIGNMENT = 0x1000;

//////////////////////////////////////////////////////////////////////////////

enum SH_TYPE
{
    SHT_NULL     = 0x0,
    SHT_PROGBITS = 0x1,
    SHT_SYMTAB   = 0x2,
    SHT_STRTAB   = 0x3,
    SHT_RELA     = 0x4,
};

enum SH_FLAGS
{
    SHF_WRITE     = 0x1,
    SHF_ALLOC     = 0x2,
    SHF_EXECINSTR = 0x4,
    SHF_INFO_LINK = 0x40,
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

enum ST_BIND
{
    STB_LOCAL  = 0x0,
    STB_GLOBAL = 0x1,
    STB_WEAK   = 0x2,
};

enum ST_TYPE
{
    STT_NOTYPE  = 0x0,
    STT_OBJECT  = 0x1,
    STT_FUNC    = 0x2,
    STT_SECTION = 0x3,
    STT_FILE    = 0x4,
};

enum ST_VISIBILITY
{
    STV_DEFAULT = 0x0,
};

struct Elf64_Sym 
{
	Elf64_Word    st_name;
	unsigned char st_info;
	unsigned char st_other;
	Elf64_Half    st_shndx;
	Elf64_Addr    st_value;
	Elf64_Xword   st_size;
};

//////////////////////////////////////////////////////////////////////////////

enum R_TYPE
{
    R_X86_64_PC32  = 0x2,
    R_X86_64_PLT32 = 0x4,
};

struct Elf64_Rela
{
    Elf64_Addr   r_offset;
    Elf64_Xword  r_info;
    Elf64_Sxword r_addend;
};

//////////////////////////////////////////////////////////////////////////////

struct Section
{
    Elf64_Shdr  shdr;

    const char* name;

    Buffer      buffer;     // section body

    uint64_t    offset;     // offset in bytes from file beginning
    uint64_t    descriptor; // index of section in Binary
};

struct Binary
{
    Elf64_Ehdr  ehdr;           // ELF-header


    Elf64_Shdr* shdrs;          // section headers
    uint64_t    shdrs_offset;   //

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
int binary_generate_rela  (Section* relatbl, Symtable* table, Relocations* relocs);
int binary_generate_strtab(Section* strtab,  Symtable* table);
int binary_generate_symtab(Section* symtab,  Symtable* table);

void binary_reserve_hdrs(Binary* bin);

int binary_arrange_sections(Binary* bin);

int binary_generate_shdrs(Binary* bin);
int binary_generate_ehdr(Binary* bin);

int binary_write(FILE* stream, Binary* bin);

void binary_dtor(Binary* bin);

#endif // ELF_WRAP_H
