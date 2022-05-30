#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "../token/Token.h"
#include "depend.h"
#include "../../include/logs/logs.h"

static const char DEP_SUFFIX[] = ".dep";

// Converts source name to name with .dep suffix
int dep_get_filename(char** dst, const char* src)
{
    assert(src);

    const char* end = strrchr(src, '.');
    size_t len = 0;

    if(end)
        len = (size_t) (end - src);
    else
        len = strlen(src);
    
    char* ptr = (char*) calloc(len + sizeof(DEP_SUFFIX), sizeof(char));
    ASSERT_RET$(ptr, DEP_BAD_ALLOC);

    memcpy(ptr, src, len);
    memcpy(ptr + len, DEP_SUFFIX, sizeof(DEP_SUFFIX));

    *dst = ptr;

    return DEP_NOERR;
}

static int dep_resize(Dependencies* deps)
{
    assert(deps);

    size_t new_cap = deps->buffer_cap * 2;
    if(new_cap == 0)
        new_cap = 32;
    
    Dep* ptr = (Dep*) realloc(deps->buffer, new_cap * sizeof(Dep));
    ASSERT_RET$(ptr, DEP_BAD_ALLOC);

    deps->buffer     = ptr;
    deps->buffer_cap = new_cap;

    return DEP_NOERR;
}

void dep_dtor(Dependencies* deps)
{
    assert(deps);

    free(deps->buffer);

    *deps = {};
}

int dep_add(Dependencies* deps, Dep dep)
{
    assert(deps);

    if(deps->buffer_cap == deps->buffer_sz)
    {
        PASS$(!dep_resize(deps), return DEP_BAD_ALLOC; );
    }

    for(size_t iter = 0; iter < deps->buffer_sz; iter++)
    {
        ASSERT_RET$(strcmp(deps->buffer[iter].func.val.name, dep.func.val.name) != 0, DEP_ALREADY_INSERTED);
    }

    deps->buffer[deps->buffer_sz] = dep;
    deps->buffer_sz++;

    return DEP_NOERR;
}

void dep_write(Dependencies* deps, FILE* stream)
{
    assert(stream && deps);

    for(size_t iter = 0; iter < deps->buffer_sz; iter++)
    {
        Dep* ptr = &deps->buffer[iter];
        fprintf(stream, "%s %lu\n", ptr->func.val.name, ptr->n_args);
    }
}

int dep_read(Dependencies* deps, char* data, size_t file_sz)
{
    assert(data && deps);

    size_t pos = 0;
    while(pos < file_sz)
    {
        Dep dep = {};
        dep.func.type = TYPE_ID;
        dep.func.val.name = data + pos;

        int n_read = 0;
        int name_len = 0;
        ASSERT_RET$(sscanf(data + pos, "%*s%n %lu%n", &name_len, &dep.n_args, &n_read) != EOF, DEP_BAD_READ);

        data[pos + (size_t) name_len] = '\0';
        pos += (size_t) n_read;

        int err = dep_add(deps, dep);
        PASS$(!err, return err; );

        while(isspace(data[pos]))
            pos++;
    }

    return DEP_NOERR;
}
