/**
 * @author      : wuzheng 
 * @file        : arm64_instruction_types
 * @created     : Tuesday Mar 02, 2021 14:53:28 CST
 */

#include "arm64_instruction_types.h"

arm64_mask_value arm64_pc_relate_inst_mask_values[ARM64_PC_RELATE_INST_NUM] = {
    {BRANCH_UNCOND, 0x14000000, 0xfc000000},
    {BRANCH_LINK,   0x94000000, 0xfc000000},
    {BRANCH_COND,   0x54000000, 0xff000010},
    {CBNZ,          0x35000000, 0x7f000000},
    {CBZ,           0x34000000, 0x7f000000},
    {TBNZ,          0x37000000, 0xfc000000},
    {TBZ,           0x36000000, 0xfc000000},
    {ADRP,          0x90000000, 0x9f000000},
    {ADR,           0x10000000, 0x9f000000},
    {LDR_L,         0x18000000, 0xbf000000}, //literal
    {LDRSW_L,       0x98000000, 0xff000000}, //literal
    {LDR_FP_L,      0x1c000000, 0x3f000000}, //literal
};
