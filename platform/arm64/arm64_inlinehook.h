/**
 * @author      : wuzheng 
 * @file        : arm64_inlinehook
 * @created     : Monday Mar 01, 2021 14:16:17 CST
 */

#ifndef ARM64_INLINEHOOK_H
#define ARM64_INLINEHOOK_H


#include <stdint.h>
#include "../../base/processor.h"
#include "../../base/mem_manager.h"
#include "arm64_jmptable.h"

class Processor;
class Arm64InstVisitor;
class Arm64Instruction;

class Arm64InlineHook: public Processor, MemManager, Visitor{
    public:
        static Arm64InlineHook *getInstance();
        virtual ~Arm64InlineHook();
       
        virtual void visitPcRelatedInst(Instruction *inst);
        virtual void visitPcNoRelatedInst(Instruction *inst);
        virtual Instruction *decodeInst(void *code, int len, uint64_t pc);
        virtual bool process(void *code, int len, uint64_t pc);
        virtual bool finishProcess(void *code, int len, uint64_t pc);

        bool hookFunction(void *func_addr, void *hook_func, void **orig_func);
        
        void reset(); 
        void transformInstList();
        bool hasAnyDataInSplitCode();
        bool canSplit();
        void ensureEnoughSpace(uint64_t trans_code_min_sz = 24);

        void analysisRelatePC();
    protected:
        Arm64InlineHook();
    private:
        std::vector<uint64_t> mLdrAddressList;
        std::vector<uint64_t> mJmpAddressList;
        std::vector<Instruction *> mProcessInstList;
        uint64_t mDefaultSplitSize;
        uint64_t mProcessStart;
        uint64_t mProcessLen;
};




#endif /* end of include guard ARM64_INLINEHOOK_H */

