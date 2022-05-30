#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

struct Buffer
{
    uint8_t* buf;
    size_t   pos;
    size_t   size;
    size_t   cap;
};

int buffer_append_i8 (Buffer* buffer, int8_t   val);
int buffer_append_u8 (Buffer* buffer, uint8_t  val);
int buffer_append_i32(Buffer* buffer, int32_t  val);
int buffer_append_u32(Buffer* buffer, uint32_t val);
int buffer_append_u64(Buffer* buffer, uint64_t val);

int buffer_append_arr(Buffer* buffer, const void* arr, size_t len);

void buffer_seek  (Buffer* buffer, uint64_t pos);
void buffer_rewind(Buffer* buffer);

void buffer_dtor(Buffer* buffer);

#endif // BUFFER_H
