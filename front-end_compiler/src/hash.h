#ifndef HASH_H
#define HASH_H

#include <ctype.h>
#include <stdint.h>

/**
 * Get 64-bit FNV1 hash integer.
 *
 * @param data      text source data
 * @param nbytes    size of data
 *
 * @return 64-bit unsigned hash value.
 *
 * @code
 *   uint64_t fnv64 = fnv1_64((void*)"hello", 5);
 * @endcode
 */
inline uint64_t fnv1_64(const void *data, size_t nbytes) {
    if (data == nullptr || nbytes == 0)
        return 0;

    const unsigned char *dptr = nullptr;
    uint64_t h = 0xCBF29CE484222325ULL;

    for (dptr = (const unsigned char*) data; nbytes > 0; dptr++, nbytes--) {
#ifdef __GNUC__
        h = h + (h << 1) + (h << 4) + (h << 5) +
        (h << 7) + (h << 8) + (h << 40);
#else
        h *= 0x100000001B3ULL;
#endif
        h ^= *dptr;
    }

    return h;
}

#endif // HASH_H
