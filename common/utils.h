/**
 * @author      : wuzheng 
 * @file        : utils
 * @created     : Monday Mar 01, 2021 09:43:26 CST
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef PAGE_MASK
#define PAGE_MASK (~(PAGE_SIZE-1))
#endif

#ifndef PAGE_START
#define PAGE_START(x)  ((x) & PAGE_MASK)
#define PAGE_END(x)    PAGE_START((x) + (PAGE_SIZE-1))
#endif

#ifndef O_RDONLY
#define O_RDONLY 0
#endif

template<class T>
T sign_extend(T x, const int bits) {
    T m = 1;
    m <<= bits - 1;
    return (x ^ m) - m;
}


uint64_t GET8(uint8_t *p);
uint32_t GET4(uint8_t *p);
uint16_t GET2(uint8_t *p);
void SET8(uint8_t *p, uint64_t val);
void SET4(uint8_t *p, uint32_t val);
void SET2(uint8_t *p, uint16_t val);

uint32_t SignExtend32(uint32_t Data, uint32_t TopBit);
uint64_t SignExtend64(uint64_t Data, uint64_t TopBit);
void setMemoryProt(void *memory, unsigned long size, int prot);

#ifdef __aarch64__
void arm64_flush_cache (void *base, void *end);
#endif

#endif /* end of include guard UTILS_H */

