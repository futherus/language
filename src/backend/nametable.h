#ifndef NAMETABLE_H
#define NAMETABLE_H

#include "generator.h"

struct Variable 
{
    const char* id = nullptr;
    ptrdiff_t offset = 0;
    ptrdiff_t size   = 0;
    bool is_const = false;
};

struct Function
{
    const char* id = nullptr;
    ptrdiff_t n_args = 0;
};

struct Function_table
{
    ptrdiff_t size = 0;
    ptrdiff_t cap  = 0;

    Function* array = nullptr;
};

struct Variable_table
{
    ptrdiff_t size = 0;
    ptrdiff_t cap  = 0;

    Variable* array = nullptr;
};

extern const char CALL_VARIABLE[];

ptrdiff_t     vartable_end(Variable_table* table);
Variable*     vartable_find(Variable_table* table, const char* id);
generator_err vartable_add(Variable_table* table, Variable var);

void          vartable_dstr(Variable_table* table);

Function*     functable_find(Function_table* table, const char* id);
generator_err functable_add(Function_table* table, const Function* func);

void          functable_dstr(Function_table* table);

void          nametable_dump_init(FILE* dump_stream);

void          functable_dump(Function_table* table);
void          vartable_dump(Variable_table* table);

#endif // NAMETABLE_H