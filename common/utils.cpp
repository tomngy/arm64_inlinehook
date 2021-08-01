/**
 * @author      : wuzheng 
 * @file        : utils
 * @created     : Tuesday Mar 02, 2021 16:56:11 CST
 */

#include <sys/mman.h>
#include "utils.h"
#include "bits.h"
#include "hook_debug.h"

enum Endian{
    LittleEnd,
    BigEnd
};

Endian endian = LittleEnd;

uint64_t GET8(uint8_t *p){
    uint64_t ret = 0;
    if(endian == LittleEnd){
        for(int i = 0 ; i < 8; i++){
            ret += (((uint64_t)p[i]) << (8*i));
        }
    }else{
        for(int i = 0 ; i < 8; i++){
            ret += (((uint64_t)p[7-i]) << (8*i));
        }
    }
    return ret;
}


uint32_t GET4(uint8_t *p){
    if(endian == LittleEnd){
        return (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
    }else{
        return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
    }
}

uint16_t GET2(uint8_t *p){
    if(endian == LittleEnd){
        return (p[1] << 8) + p[0];
    }else{
        return (p[0] << 8) + p[1];
    }
}

void SET8(uint8_t *p, uint64_t val){
    if(endian == LittleEnd){
        for(int i = 0 ; i < 8; i++){
            p[i] = (val >> (8*i)) & 0xff;
        }
    }else{
        for(int i = 0 ; i < 8; i++){
            p[i-1] = (val >> (8*i)) & 0xff;
        }
    }
}

void SET4(uint8_t *p, uint32_t val){
    if(endian == LittleEnd){
        p[3] = (val >> 24) & 0xff;
        p[2] = (val >> 16) & 0xff;
        p[1] = (val >> 8) & 0xff;
        p[0] = (val) & 0xff;
    }else{
        p[0] = (val >> 24) & 0xff;
        p[1] = (val >> 16) & 0xff;
        p[2] = (val >> 8) & 0xff;
        p[3] = (val) & 0xff;
    }
}

void SET2(uint8_t *p, uint16_t val){
    if(endian == LittleEnd){
        p[1] = (val >> 8) & 0xff;
        p[0] = (val) & 0xff;
    }else{
        p[0] = (val >> 8) & 0xff;
        p[1] = (val) & 0xff;
    }
}

uint32_t SignExtend32(uint32_t Data, uint32_t TopBit)
{
    if (((Data & TopBit) == 0) || (TopBit == BIT31)) {
        return Data;
    }

    do {
        TopBit <<= 1;
        Data |= TopBit;
    } while ((TopBit & BIT31) != BIT31);

    return Data;
}

uint64_t SignExtend64(uint64_t Data, uint64_t TopBit)
{
    if (((Data & TopBit) == 0) || (TopBit == BIT63)) {
        return Data;
    }

    do {
        TopBit <<= 1;
        Data |= TopBit;
    } while ((TopBit & BIT63) != BIT63);

    return Data;
}


void setMemoryProt(void *memory, unsigned long size, int prot){
    unsigned long seg_start = (unsigned long)memory;
    unsigned long seg_end = seg_start + size;
    unsigned long page_start = PAGE_START(seg_start);
    unsigned long page_end = PAGE_END(seg_end);
    
    //mprotect((void *)page_start, page_end - page_start, PROT_EXEC  | PROT_READ | PROT_WRITE);
    mprotect((void *)page_start, page_end - page_start, prot);
}


#ifdef __aarch64__
void arm64_flush_cache (void *base, void *end)  
{
  unsigned icache_lsize;
  unsigned dcache_lsize;
  static unsigned int cache_info = 0;
  const char *address;
  if (! cache_info)
    asm volatile ("mrs\t%0, ctr_el0":"=r" (cache_info)); 
  icache_lsize = 4 << (cache_info & 0xF);
  dcache_lsize = 4 << ((cache_info >> 16) & 0xF);
  address = (const char*) ((__UINTPTR_TYPE__) base
			   & ~ (__UINTPTR_TYPE__) (dcache_lsize - 1));
  for (; address < (const char *) end; address += dcache_lsize)
    asm volatile ("dc\tcvau, %0"
		  :
		  : "r" (address)
		  : "memory");
  asm volatile ("dsb\tish" : : : "memory");
  address = (const char*) ((__UINTPTR_TYPE__) base
			   & ~ (__UINTPTR_TYPE__) (icache_lsize - 1));
  for (; address < (const char *) end; address += icache_lsize)
    asm volatile ("ic\tivau, %0"
		  :
		  : "r" (address)
		  : "memory");
  asm volatile ("dsb\tish; isb" : : : "memory");
}
#endif
