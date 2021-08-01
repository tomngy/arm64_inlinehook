/**
 * @author      : wuzheng 
 * @file        : instruction
 * @created     : Thursday Feb 25, 2021 16:23:11 CST
 */

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <stddef.h>

class Instruction{
    public:
        Instruction(void *code, int len, uint64_t pc, bool isData=false):
            mCode(code), 
            mLength(len), 
            mPc(pc), 
            mRelatePc(0), 
            mNewPc(pc),
            mNewDataAddr(0),
            mRelateInst(NULL),
            mType(-1),
            mbData(isData),
            mbRelatedPc(false){
        }
        virtual ~Instruction(){}
        int getType(){return mType;}
        void *getCodePtr(){return mCode;}
        uint64_t getPc(){return mPc;}
        uint64_t getRelatePc(){return mRelatePc;}
        uint64_t getNewPc(){return mNewPc;}
        uint64_t getNewDataAddr(){return mNewDataAddr;}
        Instruction *getRelateInst(){return mRelateInst;}

        virtual int length() = 0;
        virtual void parse() = 0;
        bool isRelatedPc(){return mbRelatedPc;}
        bool isData(){return mbData;}
        
        void setRelatePc(uint64_t pc){mRelatePc = pc;}
        void setNewPc(uint64_t pc){mNewPc = pc;}
        void setNewDataAddr(uint64_t addr){mNewDataAddr = addr;}
        void setRelateInst(Instruction *inst){mRelateInst = inst;}

    protected:
        void *mCode;
        int mLength;
        uint64_t mPc;
        uint64_t mRelatePc;
        uint64_t mNewPc;
        uint64_t mNewDataAddr;
        Instruction *mRelateInst; //抽离时,如果寻址地址在抽离的指令内存，关联的已抽离的指令地址
        int mType;
        bool mbData;
        bool mbRelatedPc;
};

#endif /* end of include guard INSTRUCTION_H */

