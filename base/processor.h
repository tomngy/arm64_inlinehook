/**
 * @author      : wuzheng 
 * @file        : processor
 * @created     : Thursday Feb 25, 2021 15:46:40 CST
 */

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>
#include "instruction.h"
#include "visitor.h"

class Instruction;
class Visitor;
/*
* Processor 类用于实现具体功能，如
* InlineHook 实现指令抽离，修复以及最后填写jmp table
* CodeSpilt  实现指令抽取，修复以及生成shellcode
* VMP        指令解析后进行虚拟化转换，最后填入dispach的调用
*/
class Processor{
    public:
        Processor(Visitor *visitor):mInstVisitor(visitor){}
        ~Processor(){};
        virtual bool process(void *code, int maxlen, uint64_t pc) = 0;    
        /*处理结果根据功能单独实现*/
        virtual bool finishProcess(void *code, int maxlen, uint64_t pc) = 0;    

    protected:
        /*需平台单独实现处理方法*/
        virtual Instruction *decodeInst(void *code, int len, uint64_t pc) = 0;
        Visitor *mInstVisitor;
};


#endif /* end of include guard PROCESSOR_H */

