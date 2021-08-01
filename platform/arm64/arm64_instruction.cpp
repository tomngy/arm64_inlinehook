/**
 * @author      : wuzheng 
 * @file        : arm64_instruction
 * @created     : Monday Mar 01, 2021 14:37:50 CST
 */

#include "arm64_instruction.h"
#include "arm64_instruction_types.h"
#include "../../common/hook_debug.h"

void Arm64Instruction::parse() {
    //ARM64_PC_RELATE_INST_NUM       
    uint32_t opcode = *(uint32_t *)mCode;
    for(int i = 0; i < ARM64_PC_RELATE_INST_NUM; i++){
        if((opcode & arm64_pc_relate_inst_mask_values[i].mask) == arm64_pc_relate_inst_mask_values[i].code){
            mType = arm64_pc_relate_inst_mask_values[i].type;
            mbRelatedPc = true;
            break;
        }
    }
}

