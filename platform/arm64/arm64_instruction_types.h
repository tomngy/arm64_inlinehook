/**
 * @author      : wuzheng 
 * @file        : arm64_instruction_types
 * @created     : Monday Mar 01, 2021 15:59:38 CST
 */

#ifndef ARM64_INSTRUCTION_TYPES_H
#define ARM64_INSTRUCTION_TYPES_H

#include <stdint.h>
//check op0
typedef enum{
    RESERVED1,     //0b0000
    SVE,          //Scalable Vector Extension
    BRACH,
    EXCEPTION,
    SYSTEM,
    LOAD_AND_STORE,
    DATA_PROCESSING_IMM,
    DATA_PROCESSING_REGISTER,
    DATA_PROCESSING_FP,
    SIMD
}arm64_instruction_group;

/*
typedef union{
    uint32_t code;
    struct{
        uint32_t _reserve_begin:3;
        uint32_t _class:4;
        uint32_t _reserve_end:25;
    }group_classify_st;

    struct{
        uint32_t _op0:3;
        uint32_t _class:4;
        uint32_t _op1:9;
        uint32_t _reserve_end:16;
    }class_reserve0_st;

    struct{
    }class_reserve1_st;
};
*/

typedef enum{
    BRANCH_UNCOND,
    BRANCH_LINK,
    BRANCH_COND,
    CBNZ,
    CBZ,
    TBNZ,
    TBZ,
    ADRP,
    ADR,
    LDR_L,
    LDRSW_L,
    LDR_FP_L,
    UNKNOWN
}arm64_instruction_type;

typedef struct {
    arm64_instruction_type type;
    uint32_t  code;
    uint32_t  mask;
} arm64_mask_value;

#define ARM64_PC_RELATE_INST_NUM 12
extern arm64_mask_value arm64_pc_relate_inst_mask_values[ARM64_PC_RELATE_INST_NUM];

#endif /* end of include guard ARM64_INSTRUCTION_TYPES_H */

