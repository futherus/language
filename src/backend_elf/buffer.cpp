#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "buffer.h"

static int buffer_resize(Buffer* buffer)
{
    assert(buffer);

    if(buffer->cap < 1024)
        buffer->cap = 1024;
    
    size_t new_cap = buffer->cap * 2;
    
    uint8_t* ptr = (uint8_t*) realloc(buffer->buf, new_cap * sizeof(uint8_t));
    assert(ptr && "Allocation failed");

    buffer->buf = ptr;
    buffer->cap = new_cap;

    return 0;
}

int buffer_append_u8(Buffer* buffer, uint8_t val)
{
    assert(buffer);

    if(buffer->buf == nullptr || buffer->cap <= buffer->size)
        buffer_resize(buffer);

    buffer->buf[buffer->size] = val;
    buffer->size += sizeof(uint8_t);

    return 0;
}

int buffer_append_u64(Buffer* buffer, uint64_t val)
{
    assert(buffer);

    if(buffer->buf == nullptr || buffer->cap <= buffer->size + sizeof(uint64_t))
        buffer_resize(buffer);

    memcpy(buffer->buf + buffer->size, &val, sizeof(uint64_t));
    buffer->size += sizeof(uint64_t);

    return 0;
}

int buffer_append_i32(Buffer* buffer, int32_t val)
{
    assert(buffer);

    if(buffer->buf == nullptr || buffer->cap == 0)
        buffer_resize(buffer);

    memcpy(buffer->buf + buffer->size, &val, sizeof(int32_t));
    buffer->size += sizeof(int32_t);

    return 0;
}

int buffer_append_u32(Buffer* buffer, uint32_t val)
{
    assert(buffer);

    if(buffer->buf == nullptr || buffer->cap <= buffer->size + sizeof(uint32_t))
        buffer_resize(buffer);

    memcpy(buffer->buf + buffer->size, &val, sizeof(uint32_t));
    buffer->size += sizeof(uint32_t);

    return 0;
}

int buffer_append_arr(Buffer* buffer, const void* arr, size_t len)
{
    assert(buffer && arr);

    while(buffer->buf == nullptr || buffer->cap <= buffer->size + len)
        buffer_resize(buffer);

    memcpy(buffer->buf + buffer->size, arr, len);
    buffer->size += len;

    return 0;
}

void buffer_dtor(Buffer* buffer)
{
    assert(buffer);

    free(buffer->buf);

    *buffer = {};
}
