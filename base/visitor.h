/**
 * @author      : wuzheng 
 * @file        : visitor
 * @created     : Monday Mar 01, 2021 10:56:58 CST
 */

#ifndef VISITOR_H
#define VISITOR_H

#include <stdint.h>
#include "instruction.h"

class Instruction;

class Visitor{
    public:
        Visitor(){};
        ~Visitor(){};
        void visit(Instruction *inst){
            if(inst->isRelatedPc()){
                return visitPcRelatedInst(inst);
            }
            return visitPcNoRelatedInst(inst);
        };
        virtual void visitPcRelatedInst(Instruction *inst) = 0;
        virtual void visitPcNoRelatedInst(Instruction *inst) = 0;
};

#endif /* end of include guard VISITOR_H */
