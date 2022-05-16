#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "buffer.h"

static size_t BUFFER_CHUNK_SIZE = 1023;

static int buffer_resize(Buffer* buffer, size_t new_cap)
{
    assert(buffer);

    new_cap = (new_cap / (BUFFER_CHUNK_SIZE + 1) + 1) * BUFFER_CHUNK_SIZE;

    uint8_t* ptr = (uint8_t*) realloc(buffer->buf, new_cap * sizeof(uint8_t));
    assert(ptr && "Allocation failed");

    buffer->buf = ptr;
    buffer->cap = new_cap;

    return 0;
}

#define DEFINE_BUFFER_APPEND(NAME, TYPE)                                         \
    int buffer_append_##NAME(Buffer* buffer, TYPE val)                           \
    {                                                                            \
        assert(buffer);                                                          \
                                                                                 \
        if(buffer->buf == nullptr || buffer->cap <= buffer->size + sizeof(TYPE)) \
            buffer_resize(buffer, buffer->cap * 2);                              \
                                                                                 \
        memcpy(buffer->buf + buffer->pos, &val, sizeof(TYPE));                   \
        buffer->pos += sizeof(TYPE);                                             \
        if(buffer->pos > buffer->size)                                           \
            buffer->size = buffer->pos;                                          \
                                                                                 \
        return 0;                                                                \
    }                                                                            \

DEFINE_BUFFER_APPEND(i8,  int8_t)
DEFINE_BUFFER_APPEND(u8,  uint8_t)
DEFINE_BUFFER_APPEND(i32, int32_t)
DEFINE_BUFFER_APPEND(u32, uint32_t)
DEFINE_BUFFER_APPEND(u64, uint64_t)

#undef DEFINE_BUFFER_APPEND

int buffer_append_arr(Buffer* buffer, const void* arr, size_t len)
{
    assert(buffer && arr);

    if(buffer->buf == nullptr || buffer->cap <= buffer->size + len)
        buffer_resize(buffer, buffer->cap + len);

    memcpy(buffer->buf + buffer->pos, arr, len);
    buffer->pos += len;
    if(buffer->pos > buffer->size)
        buffer->size = buffer->pos;

    return 0;
}

void buffer_seek(Buffer* buffer, uint64_t pos)
{
    assert(buffer);
    assert(buffer->size > pos);

    buffer->pos = pos;
}

void buffer_rewind(Buffer* buffer)
{
    assert(buffer);

    buffer->pos = buffer->size;
}

void buffer_dtor(Buffer* buffer)
{
    assert(buffer);

    free(buffer->buf);

    *buffer = {};
}
