/**
 * @author      : wuzheng 
 * @file        : mem_manager
 * @created     : Monday Mar 01, 2021 14:04:10 CST
 */

#ifndef mem_manager_H
#define mem_manager_H

#include <vector>
#include <stdint.h>

class MemManager{
    public:
        MemManager(){
            allocNewSpace();
            mOrigPcStart = mCurrentPcStart;
            mOrigDataEnd = mCurrentDataEnd;
        }

        virtual ~MemManager();

        void allocNewSpace();
        uint64_t getPcStart(){return mCurrentPcStart;}
        uint64_t getDataEnd(){return mCurrentDataEnd;}
#ifdef DEBUG_HOOK
        void dump();
#endif
    protected:
        std::vector<void *> mAllocAddress;
        uint64_t mCurrentPcStart;
        uint64_t mCurrentDataEnd;
        uint64_t mOrigPcStart;
        uint64_t mOrigDataEnd;
};


#endif /* end of include guard mem_manager_H */

