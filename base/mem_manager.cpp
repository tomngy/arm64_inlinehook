/**
 * @author      : wuzheng 
 * @file        : mem_manager
 * @created     : Monday Mar 01, 2021 14:09:21 CST
 */

#include <malloc.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "mem_manager.h"
#include "../common/hook_debug.h"

void MemManager::allocNewSpace(){
    void *addr = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(addr == MAP_FAILED){
        int fd = open("/dev/zero", O_RDONLY);
        addr = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, fd, 0);
        close(fd);
    }
    if(addr == MAP_FAILED){
        LOG("failed alloc space");
        abort();
    }
    memset(addr, 0, 0x1000);
    uint32_t *p = (uint32_t *)addr;
    for(int i= 0; i < 0x1000/4; i ++){
        p[i] = 0;//0xaa1103f1;
    }
    mAllocAddress.push_back(addr);
    mCurrentPcStart = (uint64_t)addr;
    mCurrentDataEnd = mCurrentPcStart + 0x1000;
    //mCurrentDataEnd = mCurrentPcStart + 56; //最小长度要比MIN_SPLITE_SIZE + 24 + 4要大，不然可能产生越界
    LOG(">>>>>>>>> alloc space success : mCurrentPcStart %lx, mCurrentDataEnd %lx", mCurrentPcStart, mCurrentDataEnd);
}

MemManager::~MemManager(){
    for(std::vector<void *>::iterator itr = mAllocAddress.begin(); itr != mAllocAddress.end(); itr++){
        if((*itr) != NULL){
            LOG("unmap alloc space %p", *itr);
            munmap(*itr, 0x1000);
        }
    }
    mAllocAddress.clear();
}

#ifdef DEBUG_HOOK
void MemManager::dump(){
        for(std::vector<void *>::iterator itr = mAllocAddress.begin(); itr != mAllocAddress.end(); itr++){
        if((*itr) != NULL){
            uint32_t *p = (uint32_t *)(*itr);
            for(int i = 0; i < 0x1000/4; i++){
                if(p[i] == 0) break;
                LOG("dump inst %p : %x", p+i, p[i]);
            }
        }
    }
}
#endif

