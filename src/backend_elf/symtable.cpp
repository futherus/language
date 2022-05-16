#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "symtable.h"
#include "elf.h"

///////////////////////////////////////////////////////////////////////////////

static int symtable_resize(Symtable* tbl, size_t new_cap)
{
    assert(tbl);

    assert(new_cap > tbl->buffer_cap);

    Symbol* ptr = (Symbol*) realloc(tbl->buffer, new_cap * sizeof(Symbol));
    assert(ptr);

    tbl->buffer     = ptr;
    tbl->buffer_cap = new_cap;

    return 0;
}

int symtable_ctor(Symtable* tbl)
{
    assert(tbl);
    *tbl = {};

    symtable_resize(tbl, 32);

    return 0;
}

void symtable_dtor(Symtable* tbl)
{
    assert(tbl);

    free(tbl->buffer);

    *tbl = {};
}

int symtable_find(Symtable* tbl, const char* key, Symbol* ret, uint64_t* retindex)
{
    assert(tbl && key);

    for(size_t iter = 0; iter < tbl->buffer_sz; iter++)
    {
        if(key == tbl->buffer[iter].id) // all id's are in nametable, ptr equality => id's equality
        {
            if(ret)
                *ret = tbl->buffer[iter];
            if(retindex)
                *retindex = iter;
            
            return 0;
        }
    }

    return 1;
}

int symtable_insert(Symtable* tbl, Symbol sym, uint64_t* retindex)
{
    assert(tbl);

    assert(symtable_find(tbl, sym.id) != 0);

    if(tbl->buffer_cap == tbl->buffer_sz)
        symtable_resize(tbl, tbl->buffer_cap * 2);
    
    if(retindex)
        *retindex = tbl->buffer_sz;

    tbl->buffer[tbl->buffer_sz] = sym;
    tbl->buffer_sz++;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

const char CALL_PARAMETER[] = "(((__call_parameter__)))";

static int localtable_resize(Localtable* tbl, size_t new_cap)
{
    assert(tbl);

    assert(new_cap > tbl->buffer_cap);

    Local_var* ptr = (Local_var*) realloc(tbl->buffer, new_cap * sizeof(Local_var));
    assert(ptr);

    tbl->buffer     = ptr;
    tbl->buffer_cap = new_cap;

    return 0;
}

int localtable_ctor(Localtable* tbl)
{
    assert(tbl);
    *tbl = {};
    tbl->offset_bottom = 16;

    localtable_resize(tbl, 32);

    return 0;
}

void localtable_dtor(Localtable* tbl)
{
    assert(tbl);

    free(tbl->buffer);

    *tbl = {};
}

int localtable_find(Localtable* tbl, const char* key, Local_var* ret)
{
    assert(tbl && key);
    
    for(size_t iter = 0; iter < tbl->buffer_sz; iter++)
    {
        if(key == tbl->buffer[iter].id) // all id's are in nametable, ptr equality => id equality
        {
            if(ret)
                *ret = tbl->buffer[iter];

            return 0;
        }
    }

    return 1;
}

int localtable_allocate(Localtable* tbl, Local_var* var)
{
    assert(tbl && var);

    assert(localtable_find(tbl, var->id) == 1);

    if(tbl->buffer_cap == tbl->buffer_sz)
        localtable_resize(tbl, tbl->buffer_cap * 2);
    
    assert(var->size < INT32_MAX);
    var->offset = tbl->offset_top - (int32_t) var->size * 8;
    tbl->buffer[tbl->buffer_sz] = *var;
    tbl->buffer_sz++;
    tbl->offset_top -= (int32_t) var->size * 8;

    return 0;
}

int localtable_allocate_argument(Localtable* tbl, Local_var* var)
{
    assert(tbl && var);
    
    if(tbl->buffer_cap == tbl->buffer_sz)
        localtable_resize(tbl, tbl->buffer_cap * 2);

    assert(var->size < INT32_MAX);
    var->offset = tbl->offset_top - (int32_t) var->size * 8;
    var->id     = CALL_PARAMETER;
    tbl->buffer[tbl->buffer_sz] = *var;
    tbl->buffer_sz++;
    tbl->offset_top -= (int32_t) var->size * 8;

    return 0;
}

int localtable_deallocate(Localtable* tbl, size_t count)
{
    assert(tbl);

    if(count > tbl->buffer_sz)
        assert(0 && "Deallocation count is greater than amount of locals");

    if(tbl->buffer[tbl->buffer_sz - count].offset > 0)
        assert(0 && "Cannot deallocate function parameters");    
    
    tbl->buffer_sz -= count;
    tbl->offset_top = tbl->buffer[tbl->buffer_sz].offset;

    return 0;
}

int localtable_set_parameter(Localtable* tbl, Local_var* var)
{
    assert(tbl && var);

    assert(!localtable_find(tbl, var->id));

    if(tbl->buffer_cap == tbl->buffer_sz)
        localtable_resize(tbl, tbl->buffer_cap * 2);

    assert(tbl->offset_bottom >= 16 && "Smashing saved rbp or return address");

    var->offset = tbl->offset_bottom;
    tbl->buffer[tbl->buffer_sz] = *var;
    tbl->buffer_sz++;
    assert(var->size < INT32_MAX);
    tbl->offset_bottom = var->offset + (int32_t) var->size;

    return 0;
}

void localtable_clean(Localtable* tbl)
{
    assert(tbl);

    tbl->buffer_sz     =  0;
    tbl->offset_top    =  0;
    tbl->offset_bottom = 16;
}
