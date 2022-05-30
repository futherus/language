#ifndef DEPEND_H
#define DEPEND_H

enum dep_err
{
    DEP_NOERR = 0,
    DEP_ALREADY_INSERTED = 1,
    DEP_BAD_ALLOC = 2,
    DEP_BAD_READ = 3,
};

struct Dep
{
    Token func;
    size_t n_args;
};

struct Dependencies
{
    Dep*   buffer       = nullptr;
    size_t buffer_sz  = 0;
    size_t buffer_cap = 0;
};

int dep_get_filename(char** dst, const char* src);

int  dep_add  (Dependencies* deps, Dep dep);
void dep_write(Dependencies* deps, FILE* stream);
int  dep_read (Dependencies* deps, char* data, size_t file_sz);
void dep_dtor (Dependencies* deps);

#endif // DEPEND_H