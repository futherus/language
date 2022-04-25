#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "nametable.h"
#include "../common/dumpsystem.h"

static const ptrdiff_t NAMETABLE_MIN_CAP     = 8;
static const ptrdiff_t NAMETABLE_CAP_MULTPLR = 2;

const char CALL_VARIABLE[] = "(((__call_variable__)))";

Variable* vartable_find(Variable_table* table, const char* id)
{
    assert(table && id);

    if(id == CALL_VARIABLE)
        return nullptr;

    for(ptrdiff_t iter = 0; iter < table->size; iter++)
    {
        if(id == table->array[iter].id)
            return &table->array[iter];
    }

    return nullptr;
}

ptrdiff_t vartable_end(Variable_table* table)
{
    assert(table);

    if(table->size == 0)
        return 0;
    
    Variable* ptr = &table->array[table->size - 1];
    return ptr->offset + ptr->size;
}

static generator_err vartable_resize_(Variable_table* table)
{
    assert(table);

    ptrdiff_t new_cap = 0;
    if(table->cap == 0)
        new_cap = NAMETABLE_MIN_CAP;
    else
        new_cap = table->cap * NAMETABLE_CAP_MULTPLR;
    
    Variable* new_arr = (Variable*) realloc(table->array, (size_t) new_cap * sizeof(Variable));
    ASSERT_RET$(new_arr, GENERATOR_BAD_ALLOC);

    table->array = new_arr;
    table->cap   = new_cap;

    return GENERATOR_NOERR;
}

generator_err vartable_add(Variable_table* table, Variable var)
{
    assert(table);
    assert(vartable_find(table, var.id) == nullptr);

    if(table->cap == table->size)
        vartable_resize_(table);
    
    if(var.size == 0)
        var.size = 1;
    
    if(table->size > 0)
    {
        Variable* prev = &table->array[table->size - 1];
        var.offset = prev->offset + prev->size;
    }
    else
    {
        var.offset = 0;
    }

    table->array[table->size] = var;
    table->size++;

    return GENERATOR_NOERR;
}

void vartable_dstr(Variable_table* table)
{
    assert(table);

    free(table->array);
    table->array = nullptr;
}

Function* functable_find(Function_table* table, const char* id)
{
    assert(table && id);

    for(ptrdiff_t iter = 0; iter < table->size; iter++)
    {
        if(id == table->array[iter].id)
            return &table->array[iter];
    }

    return nullptr;
}

static generator_err functable_resize_(Function_table* table)
{
    assert(table);

    ptrdiff_t new_cap = 0;
    if(table->cap == 0)
        new_cap = NAMETABLE_MIN_CAP;
    else
        new_cap = table->cap * NAMETABLE_CAP_MULTPLR;
    
    Function* new_arr = (Function*) realloc(table->array, (size_t) new_cap * sizeof(Function));
    ASSERT_RET$(new_arr, GENERATOR_BAD_ALLOC);

    table->array = new_arr;
    table->cap   = new_cap;

    return GENERATOR_NOERR;
}

generator_err functable_add(Function_table* table, const Function* func)
{
    assert(table);
    assert(functable_find(table, func->id) == nullptr);

    if(table->cap == table->size)
        PASS$(!functable_resize_(table), return GENERATOR_BAD_ALLOC; );
    
    table->array[table->size] = *func;
    table->size++;

    return GENERATOR_NOERR;
}

void functable_dstr(Function_table* table)
{
    assert(table);

    free(table->array);
    table->array = nullptr;
}
