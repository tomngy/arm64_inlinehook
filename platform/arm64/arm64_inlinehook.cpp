/**
 * @author      : wuzheng 
 * @file        : arm64_inlinehook
 * @created     : Monday Mar 01, 2021 14:20:33 CST
 */

#include <algorithm>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "arm64_inlinehook.h"
#include "arm64_instruction.h"
#include "arm64_instruction_types.h"
#include "common/utils.h"
#include "common/hook_debug.h"
#include "common/bits.h"

/*
* 0x00 : ldr x17, =addr;
* 0x04 : br x17;
* 0x08 : b 0x00
* 0x0c : addr[8]
* or
* 0x00 : adrp x17, page@addr
* 0x04 : add x17, addr & 0xfff
* 0x08 : br x17
*/

#define SPLIT_BY_LDR_SIZE 20
#define SPLIT_BY_ADRP_SIZE 12
#pragma pack(1)
union union_code{
    uint32_t code;
    struct{
        uint16_t rt:5;
        uint32_t imm19:19;
        uint16_t op:8;
    } ldr_literal_inst;

    struct{
        uint16_t rt:5;
        uint32_t immhi:19;
        uint16_t op2:5;
        uint8_t immlo:2;
        uint8_t op:1;
    }adr_inst;

    struct{
        uint16_t rt:5;
        uint16_t rn:5;
        uint32_t op:22;
    } ldr_register_inst;

    struct{
        uint8_t cond:4;
        uint8_t o0:1;
        uint32_t imm19:19;
        //uint8_t o1:1;
        //uint16_t op:7;
        uint16_t op:8;
    } cond_branch_inst;

    struct{
        uint32_t imm26:26;
        uint16_t op:5;
        uint8_t link:1;
    } uncond_branch_inst;

    struct{
        uint16_t rm:5;
        uint16_t rt:5;
        uint32_t op:22;
    } register_branch_inst;

    struct{
        uint16_t rt:5;
        uint32_t imm19:19;
        uint16_t op:8;
    } cbz_inst;

    struct{
        uint16_t rt:5;
        uint32_t imm14:14;
        uint16_t op:13;
    } tbz_inst;

    struct{
        uint16_t rd:5;
        uint16_t rn:5;
        uint32_t imm12:12;
        uint8_t sh:1;
        uint16_t op:8;
        uint8_t sf:1;   
    }add_inst;
};

bool can_use_adrp(uint64_t pc, uint64_t target_addr){
    uint64_t offset = 0;

    if(target_addr < pc){
        offset = PAGE_START(pc) - PAGE_START(target_addr);
    }else{
        offset = PAGE_START(target_addr) - PAGE_START(pc);
    }

    //LOG("pc %lx, target_addr %lx, offset %lx < %lx", (pc), (target_addr), offset, BIT32);
    return offset < BIT32;
}

static int get_ldr_data_size(uint32_t code){
    union_code ucode;
    ucode.code = code;
    switch(ucode.ldr_literal_inst.op){
        case 0x1c:
        case 0x98:
        case 0x18:
            return 32;
        case 0x58:
        case 0x5c:
            return 64;
        case 0x9c:
            return 128;
        default:
            return 64;
    }
    return 0;
}

static uint32_t adr_register(int regN, uint64_t pc, uint64_t target_offset){
/*
    struct{
        uint16_t rt:5;
        uint32_t immhi:19;
        uint16_t op2:5;
        uint8_t immlo:2;
        uint8_t op:1;
    }adr_inst;
*/
    union_code ucode;
    ucode.adr_inst.op = 0;
    ucode.adr_inst.op2 = 0x10;
    ucode.adr_inst.rt = regN;
    uint32_t imm = (target_offset - pc) & 0x7ffff;
    ucode.adr_inst.immlo = imm & 0b11;
    ucode.adr_inst.immhi = imm >> 2;
    return ucode.code; 
}

static uint32_t adrp_register(int regN, uint64_t pc, uint64_t target_offset){
/*
    struct{
        uint16_t rt:5;
        uint32_t immhi:19;
        uint16_t op2:5;
        uint8_t immlo:2;
        uint8_t op:1;
    }adr_inst;
*/
    union_code ucode;
    ucode.adr_inst.op = 1;
    ucode.adr_inst.op2 = 0x10;
    ucode.adr_inst.rt = regN;
    uint32_t imm = (PAGE_START(target_offset) - PAGE_START(pc)) >> 12;
    ucode.adr_inst.immlo = imm & 0b11;
    ucode.adr_inst.immhi = imm >> 2;
    return ucode.code; 
}

static uint32_t ldr_literal(int regN, uint64_t pc, uint64_t target_offset, \
        bool useFP = false, uint32_t load_bytes = 64, bool is_signed = false){
    union_code ucode;
    ucode.ldr_literal_inst.imm19 = ((target_offset - pc) >> 2 ) & 0x7ffff;
    ucode.ldr_literal_inst.rt = regN;

    if(useFP){
        switch(load_bytes){
            case 32:
                ucode.ldr_literal_inst.op = 0x1c;
                break;
            case 64:
                ucode.ldr_literal_inst.op = 0x5c;
                break;
            case 128:
                ucode.ldr_literal_inst.op = 0x9c;
                break;
            default:
                return -1;
                break;
        }
    }else{
         switch(load_bytes){
            case 32:
                ucode.ldr_literal_inst.op = is_signed ? 0x98 : 0x18;
                break;
            case 64:
                ucode.ldr_literal_inst.op = 0x58;
                break;
            default:
                return -1;
                break;
        }
        
    } 
    //LOG("----code %x, op %x, imm19 %x-----", ucode.code, ucode.ldr_literal_inst.op, ucode.ldr_literal_inst.imm19);
    return ucode.code;
}

static uint32_t ldr_rt_rn(int rt, int rn){
    union_code ucode;
    /*
       struct{
       uint16_t rt:5;
       uint32_t rn:5;
       uint16_t op:22;
       } ldr_register_inst;
    */
    ucode.ldr_register_inst.op = 0x3E5000;
    ucode.ldr_register_inst.rt = rt;
    ucode.ldr_register_inst.rn = rn;
    return ucode.code; 
}

static uint32_t add_rt_rn_imm(int rd, int rn, int imm,  bool addPage = false, int bits = 64){
    /*
        struct{
        uint16_t rd:5;
        uint16_t rn:5;
        uint32_t imm12:12;
        uint8_t sh:1;
        uint16_t op:8;
        uint8_t sf:1;   
    }add_inst;
    */
    union_code ucode;
    ucode.add_inst.sf = (bits == 64);
    ucode.add_inst.sh = addPage ? 1 : 0;
    ucode.add_inst.op = 0x22;
    ucode.add_inst.rd = rd;
    ucode.add_inst.rn = rn;

    if(addPage){
        imm = imm << 12;
    }
    ucode.add_inst.imm12 = imm & 0xfff;
    return ucode.code; 
}

static uint32_t branch_register(uint64_t rt, bool link = false){
    union_code ucode;
    ucode.register_branch_inst.op = link ? 0x358FC0 : 0x3587C0;
    ucode.register_branch_inst.rt = rt;
    ucode.register_branch_inst.rm = 0;
    return ucode.code;
}

static uint32_t nop(){
    return 0xd503201f;
}

static uint32_t uncond_branch(uint64_t pc, uint64_t target_offset, bool link = false){
    union_code ucode;
    ucode.uncond_branch_inst.link = link;
    ucode.uncond_branch_inst.op = 0x5;
    ucode.uncond_branch_inst.imm26 = ((target_offset - pc) >> 2) & 0x3ffffff;
    return ucode.code;
}

static Arm64InlineHook *s_hookmgr = NULL;
Arm64InlineHook* Arm64InlineHook::getInstance(){
    if(s_hookmgr == NULL){
        s_hookmgr = new Arm64InlineHook();
    }
    return s_hookmgr;
}

Arm64InlineHook::Arm64InlineHook():
    Processor(this){
    reset();
}

Arm64InlineHook::~Arm64InlineHook(){
    reset();
}

void Arm64InlineHook::transformInstList(){
    //transform ldr
    //transform b/bl 
    for(auto inst:mProcessInstList){
        uint32_t code = *((uint32_t *)inst->getCodePtr());
        union_code ucode;
        ucode.code = code;
        if(mCurrentDataEnd % 8 != 0){
            mCurrentDataEnd -= 4;
        }
        if(inst->isRelatedPc()){
            int cond = 0;
            switch(inst->getType()){
                //branch
                case BRANCH_UNCOND:
                case BRANCH_LINK:
                    {
                        uint64_t target_addr = inst->getRelatePc();
                        if(inst->getRelateInst() != NULL){
                            //如果目标地址在抽离的地址内，修改ldr的偏移为抽取指令的地址,还需要区分指令是否有做变换
                            target_addr = inst->getRelateInst()->getNewPc();
                        }
                        uint32_t *p = (uint32_t *)inst->getNewPc();
                        p[0] = ldr_literal(17, inst->getNewPc(), inst->getNewDataAddr());  // ldr x17, =addr
                        p[1] = branch_register(17, inst->getType() == BRANCH_LINK);   // br/bl x17
                        SET8((uint8_t *)(inst->getNewDataAddr()), target_addr);
                    }
                    break;
                case BRANCH_COND:
                case CBNZ:
                case CBZ:
                    {
                        uint64_t target_addr = inst->getRelatePc();
                        if(inst->getRelateInst() != NULL){
                            //如果目标地址在抽离的地址内，修改ldr的偏移为抽取指令的地址,还需要区分指令是否有做变换
                            target_addr = inst->getRelateInst()->getNewPc();
                        }
                        uint32_t *p = (uint32_t *)inst->getNewPc();
                        ucode.cond_branch_inst.imm19 = 2;//0x8 >> 2;
                        p[0] = ucode.code;                                                               // b.cond/cbnz/cbz pc+8
                        p[1] = uncond_branch(inst->getNewPc()+4, inst->getNewPc()+16);  // b 0x0c
                        p[2] = ldr_literal(17, inst->getNewPc()+8, inst->getNewDataAddr());   // ldr x17, =addr
                        p[3] = branch_register(17);                                                  // br x17
                        //*(uint64_t *)(mCurrentDataEnd) = inst->getRelatePc();                          // addr
                        SET8((uint8_t *)(inst->getNewDataAddr()), target_addr);
                    }
                    break;
                case TBNZ:
                case TBZ: 
                    {
                        uint64_t target_addr = inst->getRelatePc();
                        if(inst->getRelateInst() != NULL){
                            //如果目标地址在抽离的地址内，修改ldr的偏移为抽取指令的地址,还需要区分指令是否有做变换
                            target_addr = inst->getRelateInst()->getNewPc();
                        }
                        uint32_t *p = (uint32_t *)inst->getNewPc();
                        ucode.tbz_inst.imm14 = 2; // 0x8 >> 2;
                        p[0] = ucode.code;                                              // tbz/tbnz pc+8
                        p[1] = uncond_branch(inst->getNewPc()+4, inst->getNewPc()+16);  // b 0x0c
                        p[2] = ldr_literal(17, inst->getNewPc()+8, inst->getNewDataAddr());  // ldr x17, =addr 
                        p[3] = branch_register(17);                                     // br x17
                        SET8((uint8_t *)(inst->getNewDataAddr()), target_addr);
                    }
                    break;
                case ADRP:
                case ADR:
                    {
                        uint64_t target_addr = inst->getRelatePc();
                        if(inst->getRelateInst() != NULL){
                            //如果目标地址在抽离的地址内，修改ldr的偏移为抽取指令的地址,还需要区分指令是否有做变换
                            target_addr = inst->getRelateInst()->getNewPc();
                        }
                        uint32_t *p = (uint32_t *)inst->getNewPc();
                        p[0] = ldr_literal(ucode.ldr_literal_inst.rt, inst->getNewPc(), inst->getNewDataAddr());
                        SET8((uint8_t *)(inst->getNewDataAddr()), target_addr);
                    }
                    break;
                case LDR_L:
                case LDRSW_L:
                case LDR_FP_L:
                    {   //ldr的地址的数据，不能作为指令去执行
                        //一般不会有这一类指令，inlinehook代码可能有，需要考虑兼容inlinehook的代码
                        uint64_t target_addr = inst->getRelatePc();
                        uint32_t *p = (uint32_t *)inst->getNewPc();
                        p[0] = ldr_literal(ucode.ldr_literal_inst.rt, inst->getNewPc(), inst->getNewDataAddr()); // ldr rt, =taget_addr
                        //             // ldr rt, [rt]
                        if(inst->getRelateInst() != NULL){
                            //将数据拷贝
                            p[1] = nop();
                            SET8((uint8_t *)(inst->getNewDataAddr()), GET8((uint8_t*)(inst->getRelateInst()->getPc())));
                        }else{
                            p[1] = ldr_rt_rn(ucode.ldr_literal_inst.rt, ucode.ldr_literal_inst.rt);
                            SET8((uint8_t *)(inst->getNewDataAddr()), target_addr);
                        }
                    }
                    break;
                default:
                    break;
            }    
        }else{
            uint32_t *pc = (uint32_t *)(inst->getNewPc());
            memcpy(pc, inst->getCodePtr(), inst->length());
        }
    }
}

//处理过程中，如果空间不足，直接结束！！,插入跳转存在问题？
void Arm64InlineHook::ensureEnoughSpace(uint64_t trans_code_min_sz){
    static const uint64_t native_jmp_code_size = 24;
    if(mCurrentDataEnd - mCurrentPcStart < native_jmp_code_size){
        LOG("unexpected error mCurrentDataEnd(%lx)- mCurrentPcStart(%lx) < native_jmp_code_size(%lx) + 4\n", mCurrentDataEnd, mCurrentPcStart, native_jmp_code_size);
        abort();
    }
    else if(mCurrentDataEnd - mCurrentPcStart < trans_code_min_sz + 4 + native_jmp_code_size){
        //填入数据和填入指令的判断长度不一致，数据至少保证8字节的长度 + 4 + 24,如何防止刚好写入一半数据的情况
        //新创建的块大小一定 >= 58
        //至少保证能转换一条指令（最大24字节（tbz/cbz）+4字节对齐）+一条跳转指令的长度(最大24字节，6条指令)
        //add jmp code
        uint64_t old_pc = mCurrentPcStart;
        uint64_t old_data = mCurrentDataEnd-8;
        allocNewSpace();
        if(mCurrentDataEnd - mCurrentPcStart < trans_code_min_sz + 4 + native_jmp_code_size){
            LOG("unexpected error : mCurrentDataEnd - mCurrentPcStart < %lx\n", trans_code_min_sz + 4 + native_jmp_code_size);
            abort();
        }
        //0  f1 03 1f f8  stur    x17, [sp, #0xfffffffffffffff0]
        //4  f1 03 5f f8  ldur    x17, [sp, #0xfffffffffffffff0]
        uint32_t *p = (uint32_t *)old_pc;
        p[0] = 0xf81f03f1; //str x17, [sp, -0x10]
        if(can_use_adrp((uint64_t)(old_pc), (uint64_t)mCurrentPcStart )){
            p[1] = adrp_register(17, (uint64_t)(old_pc+4) , (uint64_t)mCurrentPcStart);
            p[2] = add_rt_rn_imm(17, 17, ((uint64_t)mCurrentPcStart)&0xfff);
            p[3] = branch_register(17, false);
        }else{
            p[1] = ldr_literal(17, old_pc+4, old_data);// ldr x17, =addr
            p[2] = branch_register(17, false);       // br x17
            p[3] = uncond_branch(8, 0);              // b priv inst 这条指令正常不会执行，如果跑到该指令，强制执行之前的指令，测试isb指令无效，所以只能以该方法规避
            SET8((uint8_t *)(old_data), mCurrentPcStart);
        }
        LOG("new instruction ->>>>>  %p : %x  adrp x16, %lx", p, p[1], mCurrentPcStart);
        LOG("new instruction ->>>>>  %p : %x, add x17, x16, 0", p+1, p[1]);
        LOG("new instruction ->>>>>  %p : %x, br x17", p+2, p[2]);
        LOG("\t\t\t  0x%p : %lx", (void*)old_data, mCurrentPcStart);
        p = (uint32_t *)mCurrentPcStart;
        p[0] = 0xf85f03f1;//ldr x17, [sp, -0x10]
        mCurrentPcStart += 4;
#ifdef __aarch64__
#ifdef NEED_FLUSH_CACHE
        __builtin___clear_cache((char *)PAGE_START(old_pc), (char  *)PAGE_END(old_pc));
#endif
#endif
        
    }else{
        //LOG("mCurrentPcStart %lx, mCurrentDataEnd %lx\n", mCurrentPcStart, mCurrentDataEnd);
    }
}

bool Arm64InlineHook::hasAnyDataInSplitCode(){
    uint64_t data_end = mProcessStart + mProcessLen;
    if(mProcessLen < mDefaultSplitSize){
        data_end = mProcessStart + mProcessLen;
    }
    for(auto l:mLdrAddressList){
        if(l >= mProcessStart && l < data_end){
            return true;
        }
    }
    return false;
}

bool Arm64InlineHook::canSplit(){
    //临界情况分析
    if(mProcessLen >= mDefaultSplitSize){
        return true;
    }
    //确认当前代码为数据的情况,拷贝并继续抽离
    return false;
}

Instruction *Arm64InlineHook::decodeInst(void *code, int left, uint64_t pc){
    if(left < 4){
        return NULL;
    }
    
    if(canSplit()){
        return NULL;
    }

    bool isdata = (std::find(mLdrAddressList.begin(), mLdrAddressList.end(), pc) != mLdrAddressList.end());
    Instruction *inst = new Arm64Instruction(code, 4, pc, isdata);    
    return inst;
}


bool Arm64InlineHook::process(void *code, int len, uint64_t pc){
    reset();
    mProcessStart = pc;
    unsigned char *pCode = (unsigned char *)code;
    int left = len;
    uint64_t currentPc = pc;
    while(left > 0){
        Instruction *inst = decodeInst(pCode, left, currentPc);
        if(inst == nullptr){
            break;
        }
        ensureEnoughSpace();
        mInstVisitor->visit(inst);

        pCode += inst->length();
        left -= inst->length();
        currentPc += inst->length();
        mProcessLen += inst->length();
        mProcessInstList.push_back(inst);
        //delete inst;
    }
    analysisRelatePC();
    transformInstList();
    return true;
}

void Arm64InlineHook::visitPcRelatedInst(Instruction *inst){
    LOG("%lx : %x, pc no related instruction type[%d]", inst->getPc(), *(uint32_t *)inst->getCodePtr(), inst->getType());
    if((mCurrentPcStart % 8) != (inst->getPc() % 8)){
        *((uint32_t *)mCurrentPcStart) = 0xd503201f; //NOP  1f 20 03 d5
        mCurrentPcStart += 4;
    }
    
    uint64_t target_addr = 0;
    uint32_t code = *((uint32_t *)inst->getCodePtr());
    int target_register = -1;
    bool is_data_addr = false;
    union_code ucode;
    ucode.code = code;
    switch(inst->getType()){
        case BRANCH_UNCOND:
            //target_addr = SignExtend64((code & 0x3ffffff) << 2, BIT27) + inst->getPc();
            target_addr = SignExtend64(ucode.uncond_branch_inst.imm26 << 2, BIT27) + inst->getPc();
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 8;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case BRANCH_LINK:
            //target_addr = SignExtend64((code & 0x3ffffff) << 2, BIT27) + inst->getPc();
            target_addr = SignExtend64(ucode.uncond_branch_inst.imm26 << 2, BIT27) + inst->getPc();
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 8;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case BRANCH_COND:
            //target_addr = SignExtend64(((code >> 5) & 0x7ffff) << 2, BIT20) + inst->getPc();
            target_addr = SignExtend64(ucode.cond_branch_inst.imm19 << 2, BIT20) + inst->getPc();
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 16;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case CBNZ:
            //target_addr = inst->getPc() + SignExtend64(((code >> 5) & 0x7ffff) << 2, BIT20);
            target_addr = inst->getPc() + SignExtend64(ucode.cbz_inst.imm19 << 2, BIT20);
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 16;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case CBZ:
            //target_addr = inst->getPc() + SignExtend64(((code >> 5) & 0x7ffff) << 2, BIT20);
            target_addr = inst->getPc() + SignExtend64(ucode.cbz_inst.imm19  << 2, BIT20);
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 16;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case TBNZ:
            //target_addr = inst->getPc() + SignExtend64(((code >> 5) & 0x3fff) << 2, BIT15);
            target_addr = inst->getPc() + SignExtend64(ucode.tbz_inst.imm14 << 2, BIT15);
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 16;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case TBZ:
            //target_addr = inst->getPc() + SignExtend64(((code >> 5) & 0x3fff) << 2, BIT15);
            target_addr = inst->getPc() + SignExtend64(ucode.tbz_inst.imm14 << 2, BIT15);
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 16;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case ADRP:
            target_addr = (ucode.adr_inst.immhi << 2) + ucode.adr_inst.immlo;
            target_addr = SignExtend64((target_addr << 12), BIT32);//12+21-1 32
            //LOG("ADRP hi[%x] lo[%x] %lx %lx %lx", ucode.adr_inst.immhi  + ucode.adr_inst.immlo, SignExtend64((target_addr << 12), BIT34), SignExtend64((target_addr << 12), BIT33), SignExtend64((target_addr << 12), BIT32), SignExtend64((target_addr << 12), BIT31));
            target_addr += (inst->getPc() & 0xfffffffffffff000);
            target_register = code & 0x1f;
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 4;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case ADR:
            is_data_addr = true;
            //target_addr = (((code >> 5) & 0x7ffff) << 2)
            //             | (((code >> 29)&3));
            target_addr = (ucode.adr_inst.immhi << 2) + ucode.adr_inst.immlo;
            target_addr = SignExtend64(target_addr, BIT20) + inst->getPc();
            target_register = code & 0x1f;
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 4;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case LDR_L:
            is_data_addr = true;
            //target_addr = inst->getPc() + (((code >> 5) & 0x7ffff) << 2);
            target_addr = inst->getPc() + (ucode.ldr_literal_inst.imm19 << 2);
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 8;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case LDRSW_L://signed
            is_data_addr = true;
            //target_addr = inst->getPc() + (((code >> 5) & 0x7ffff) << 2);
            target_addr = inst->getPc() + (ucode.ldr_literal_inst.imm19 << 2);
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 8;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        case LDR_FP_L:
            is_data_addr = true;
            //target_addr = inst->getPc() + (((code >> 5) & 0x7ffff) << 2);
            target_addr = inst->getPc() + (ucode.ldr_literal_inst.imm19 << 2);
            inst->setNewPc(mCurrentPcStart);
            mCurrentPcStart += 8;
            mCurrentDataEnd -= 8;
            inst->setNewDataAddr(mCurrentDataEnd);
            break;
        //case FPRM:
        //    break;
        default:
            break;
    }
    //关联PC
    inst->setRelatePc(target_addr);
    
    if(is_data_addr){
        int sz = get_ldr_data_size(code)/32;
        for(int i = 0; i < sz; i+=32){
            mLdrAddressList.push_back(target_addr + 4*i);
        }
    }

    if(target_addr != 0 && target_addr >= mProcessStart && target_addr < mProcessStart + mDefaultSplitSize){
        //TODO 跳转地址在抽离的地址段的情况，重新映射target_addr = old_addr2new_addr[old_addr]
        if(is_data_addr){
            LOG("set mDefaultSplitSize %ld -> %ld ", mDefaultSplitSize, target_addr - mProcessStart + get_ldr_data_size(code)/8);
            uint64_t resetSz = target_addr - mProcessStart + (get_ldr_data_size(code)/8); 
            mDefaultSplitSize = mDefaultSplitSize > resetSz ? mDefaultSplitSize : resetSz;
        }
    }
}

void Arm64InlineHook::analysisRelatePC(){
    for(auto inst:mProcessInstList){
        if(inst->getRelatePc() >= mProcessStart && inst->getRelatePc() < mProcessStart+mProcessLen){
            inst->setRelateInst(mProcessInstList[(inst->getRelatePc()-mProcessStart)/4]);
        }
    } 
}

void Arm64InlineHook::visitPcNoRelatedInst(Instruction *inst){
    LOG("%lx : %x, pc no related instruction ", inst->getPc(), *(uint32_t *)inst->getCodePtr());
    //抽离指令和数据的地址对齐方式和原指令一致
    if((mCurrentPcStart % 8) != (inst->getPc() % 8)){
        *((uint32_t *)mCurrentPcStart) = 0xd503201f; //NOP  1f 20 03 d5
        mCurrentPcStart += 4;
    }
    inst->setNewPc(mCurrentPcStart);
    mCurrentPcStart += 4;
}

void Arm64InlineHook::reset(){
    mOrigPcStart = mCurrentPcStart;
    mOrigDataEnd = mCurrentDataEnd;
    mProcessLen = 0;
    mProcessStart = 0;
    mLdrAddressList.clear();
    mJmpAddressList.clear();
    for(auto p:mProcessInstList){
        //LOG(" delete instructions %p", p);
        delete p;
    }
    mProcessInstList.clear();
}

bool Arm64InlineHook::finishProcess(void *code, int len, uint64_t pc){
    if(mProcessInstList.size() >= mDefaultSplitSize/4 && mProcessInstList.size() <= len/4){
        ensureEnoughSpace();
        //至少保证20字节
        uint64_t ret_addr = ((uint64_t)code) + mProcessInstList.size() * 4;
        uint32_t *p = (uint32_t *)mCurrentPcStart;

        if(can_use_adrp((uint64_t)(mCurrentPcStart), (uint64_t)ret_addr) ){
            p[0] = adrp_register(17, (uint64_t)(mCurrentPcStart), (uint64_t)ret_addr);
            p[1] = add_rt_rn_imm(17, 17, ((uint64_t)ret_addr)&0xfff);
            p[2] = branch_register(17, false);
            mCurrentPcStart += 12;
        }else{
            p[0] = ldr_literal(17, mCurrentPcStart, mCurrentDataEnd-8);  // ldr x17, =addr
            p[1] = branch_register(17, false);                                                       // br x17
            p[2] = uncond_branch(8, 0);
            SET8((uint8_t *)(mCurrentDataEnd-8), ret_addr);
            mCurrentPcStart += 12;
            mCurrentDataEnd -= 8;
        }

#ifdef __aarch64__
#ifdef NEED_FLUSH_CACHE
            __builtin___clear_cache((char *)PAGE_START(mCurrentPcStart), (char  *)PAGE_END(mCurrentPcStart));
#endif
#endif

    }else{
        mCurrentPcStart = mOrigPcStart;
        mCurrentDataEnd = mOrigDataEnd;
        return false;
    }

    return true;
}

bool Arm64InlineHook::hookFunction(void *func_addr, void *hook_func, void **orig_func){
    /*
    Dl_info info;
    int rc = dladdr((void*)func_addr, &info);
    LOG("hook %s function %s start", info.dli_fname, info.dli_sname);
     */
    //小米9 libc代码段没有读权限，hook后需要先添加读权限
    setMemoryProt(func_addr, 0x1000, PROT_EXEC  | PROT_READ);
    LOG("hook function %p -> %p", func_addr, hook_func);
    if(can_use_adrp((uint64_t)(func_addr), (uint64_t)hook_func) ){
        mDefaultSplitSize = SPLIT_BY_ADRP_SIZE;
    }else{
        mDefaultSplitSize = SPLIT_BY_LDR_SIZE;
    }

    if(func_addr == NULL) return false;
    if(process(func_addr, 24, (uint64_t)func_addr)){
        if(finishProcess(func_addr, 24, (uint64_t)func_addr)){
#ifdef DEBUG_HOOK
            dump();
#endif
            LOG("finishProcess instructions %d", (int)mProcessInstList.size());
            setMemoryProt(func_addr, 4*mProcessInstList.size(), PROT_EXEC  | PROT_READ | PROT_WRITE);
            uint32_t *p = (uint32_t *)func_addr;

           if(can_use_adrp((uint64_t)(func_addr), (uint64_t)hook_func)){
                p[0] = adrp_register(17, (uint64_t)(func_addr), (uint64_t)hook_func);
                p[1] = add_rt_rn_imm(17, 17, ((uint64_t)hook_func)&0xfff);
                p[2] = branch_register(17, false);
           }else{
               p[0] = ldr_literal(17, 0, 0x0c);        // ldr x17, =addr
               p[1] = branch_register(17, false);      // br x17
               p[2] = uncond_branch(8, 0);             // b priv inst 这条指令正常不会执行，如果跑到该指令，强制执行之前的指令，测试isb指令无效，所以只能以该方法规避
               SET8((uint8_t *)(p+3), (uint64_t)(hook_func));
           }
    
            *orig_func = (void*)mOrigPcStart;

#ifdef __aarch64__
#ifdef NEED_FLUSH_CACHE
            //arm64_flush_cache (func_addr, (void *)(((unsigned long)func_addr) + 4 * mProcessInstList.size()));
            uint64_t addr = (uint64_t)func_addr;
            __builtin___clear_cache((char *)PAGE_START(addr), (char  *)PAGE_END(addr + 4 * mProcessInstList.size()));
#endif
#endif
            
            
            LOG("JMP code %p : %x", p+0, p[0]);
            LOG("JMP code %p : %x", p+1, p[1]);
            LOG("JMP code %p : %x", p+2, p[2]);
            LOG("JMP code %p : %lx", p+3, GET8((uint8_t *)(p+3)));
            setMemoryProt(func_addr, 4*mProcessInstList.size(), PROT_EXEC  | PROT_READ);
            LOG("success hook function %p-> %p orig_func[%p]", func_addr, hook_func, *orig_func);
            
            return true;
        }
    }
    LOG("failed hook function %p", func_addr);
    return false;
}
