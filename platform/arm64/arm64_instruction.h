/**
 * @author      : wuzheng 
 * @file        : arm64_instruction
 * @created     : Monday Mar 01, 2021 14:35:10 CST
 */

#ifndef ARM64_INSTRUCTION_H
#define ARM64_INSTRUCTION_H
#include "../../base/instruction.h"

class Arm64Instruction : public Instruction{
    public:
        Arm64Instruction(void *code, int len, uint64_t pc, bool isdata=false):
            Instruction(code, len, pc){
            if(!isdata){
                parse();
            }else{
                mbData = isdata;
            }
        }
        int length(){
            return 4;
        };
        virtual void parse();
};


#endif /* end of include guard ARM64_INSTRUCTION_H */

