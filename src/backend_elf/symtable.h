#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stddef.h>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

enum Symbol_type
{
    SYMBOL_TYPE_FUNCTION = 0x1,
    SYMBOL_TYPE_VARIABLE = 0x2,
};

struct Function
{
    size_t n_args = 0;
};

struct Variable
{
    bool   is_const = false;
    size_t size     = 0;
};

struct Symbol
{
    Symbol_type type;
    const char* id = nullptr;

    // bool is_resolved;
    uint64_t offset;
    uint64_t section_descriptor;

    union
    {
        Variable var;
        Function func;
    };
};

struct Symtable
{
    Symbol*   buffer     = nullptr;
    size_t    buffer_sz  = 0;
    size_t    buffer_cap = 0;
};

int  symtable_ctor(Symtable* tbl);
void symtable_dtor(Symtable* tbl);

int  symtable_find  (Symtable* tbl, const char* key, Symbol* retsym = nullptr, uint64_t* retindex = nullptr);
int  symtable_insert(Symtable* tbl, Symbol sym, uint64_t* retindex = nullptr);

///////////////////////////////////////////////////////////////////////////////

struct Local_var
{
    const char* id;
    int32_t     offset;
    size_t      size;
    bool        is_const;
};

struct Localtable
{
    Local_var* buffer     = nullptr;
    size_t     buffer_sz  = 0;
    size_t     buffer_cap = 0;

    int32_t    offset_top    = 0; // right after variable with highest address
    int32_t    offset_bottom = 0; // offset of variable with lowest address
};

int  localtable_ctor (Localtable* tbl);
void localtable_dtor (Localtable* tbl);
void localtable_clean(Localtable* tbl);

int  localtable_find(Localtable* tbl, const char* key, Local_var* ret = nullptr);

int  localtable_allocate         (Localtable* tbl, Local_var* var);
int  localtable_allocate_argument(Localtable* tbl, Local_var* var);
int  localtable_set_parameter    (Localtable* tbl, Local_var* var);
int  localtable_deallocate       (Localtable* tbl, size_t count);

///////////////////////////////////////////////////////////////////////////////

void symtable_dump_init(FILE* dump_stream);
void localtable_dump   (Localtable* tbl);
void symtable_dump     (Symtable* tbl);

#endif // SYMTABLE_H
