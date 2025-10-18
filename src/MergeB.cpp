#include "MergeB.h"

#include <iostream>

llvm::PreservedAnalyses MergeB::run(llvm::Function& Func, llvm::FunctionAnalysisManager& ) {
    bool Changed = false;
    llvm::SmallPtrSet<llvm::BasicBlock*, 8> DeleteList;
    // attempt to merge every block with every other basically
    for (llvm::BasicBlock& BB : Func)
        Changed |= attemptMerge(&BB, DeleteList);
    for (llvm::BasicBlock* BB : DeleteList)
        DeleteDeadBlock(BB);
    return (Changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
}

bool MergeB::attemptMerge(llvm::BasicBlock* B1, llvm::SmallPtrSet<llvm::BasicBlock*, 8>& DeleteList) {
    // block cannot be the entry block of the parent function
    if (B1 == &B1->getParent()->getEntryBlock()) return false;
    // cannot be unconditional branch
    llvm::BranchInst* B1Term = llvm::dyn_cast<llvm::BranchInst>(B1->getTerminator());
    if (!(B1Term && B1Term->isUnconditional())) return false;
    // cannot be a deleted block
    if (DeleteList.contains(B1)) return false;
    // predecessor must be either branch or switch
    for (llvm::BasicBlock* B : llvm::predecessors(B1))
        if (!(llvm::isa<llvm::BranchInst>(B->getTerminator()) || llvm::isa<llvm::SwitchInst>(B->getTerminator())))
            return false;
    // defined up here so it can be reused
    unsigned int B1NumInst = countNonDbgInstrInBB(B1);
    // get first instruction of sucessor block
    llvm::BasicBlock* BBSucc = B1Term->getSuccessor(0);
    llvm::BasicBlock::iterator II = BBSucc->begin();
    const llvm::PHINode* PN = llvm::dyn_cast<llvm::PHINode>(II);
    llvm::Value* InValB1 = nullptr;
    llvm::Instruction* InInstB1 = nullptr;
    if (PN != nullptr) {
        // next instruction cannot be another phi instruction
        if ((++II != BBSucc->end()) && llvm::isa<llvm::PHINode>(II)) return false;
        // get predicate on phi instruction for this block
        InValB1 = PN->getIncomingValueForBlock(B1);
        InInstB1 = llvm::dyn_cast<llvm::Instruction>(InValB1);
    }
    for (llvm::BasicBlock* B2 : predecessors(BBSucc)) {
        // block cannot be the entry block of the parent function
        if (B2 == &B2->getParent()->getEntryBlock()) continue;
        // terminator must be unconditional
        llvm::BranchInst* B2Term = llvm::dyn_cast<llvm::BranchInst>(B2->getTerminator());
        if (!((B2Term != nullptr) && B2Term->isUnconditional())) continue;
        // terminator of predecessors blocks must be either a br or switch
        for (llvm::BasicBlock* Pb : predecessors(B2))
            if (!(llvm::isa<llvm::BranchInst>(Pb->getTerminator()) || llvm::isa<llvm::SwitchInst>(Pb->getTerminator())))
                continue;
        // cannot be a deleted block
        if (DeleteList.contains(B2)) continue;
        // cannot be equal to B1
        if (B1 == B2) continue;
        // must have the same number of instructions as B1 (just a quick check before actually checking all instructions)
        if (B1NumInst != countNonDbgInstrInBB(B2)) continue;
        // values in the phi instruction must be from above the conditional branch or defined in their respective branches
        if (nullptr != PN) {
            llvm::Value* InValB2 = PN->getIncomingValueForBlock(B2);
            llvm::Instruction* InInstB2 = llvm::dyn_cast<llvm::Instruction>(InValB2);
            bool areValuesSimilar = (InValB1 == InValB2);
            bool bothValuesDefinedInParent = ((InInstB1 != nullptr) && (InInstB1->getParent() == B1)) && ((InInstB2 != nullptr) && (InInstB2->getParent() == B2));
            if (!areValuesSimilar && !bothValuesDefinedInParent)
                continue;
        }
        // check if all instructions are mergable with one another in both blocks (see )
        if (!canMergeBlocks(B2, B1)) continue;
        // update anywhere branching to BB2 and replace it with B1
        updatePredecessorTerminator(B2, B1);
        DeleteList.insert(B2);
        return true;
    }
    return false;
}
bool MergeB::canMergeBlocks(llvm::BasicBlock* B1, llvm::BasicBlock* B2) {
    llvm::SmallVector<llvm::Instruction*, 2> Insts{};
    llvm::Instruction* InstB1 = getLastNonDbgInst(B1);
    if (InstB1 == nullptr) return true;
    llvm::Instruction* InstB2 = getLastNonDbgInst(B2);
    if (InstB2 == nullptr) return true;
    Insts.push_back(InstB1);
    Insts.push_back(InstB2);
    // get previous not untill it reaches the beginning nad check for if you are able to merge each instruction
    while (canMergeInstructions(Insts)) {
        for (llvm::Instruction*& Inst : Insts) {
            do {
                Inst = Inst->getPrevNode();
            } while (Inst && llvm::isa<llvm::DbgInfoIntrinsic>(Inst));
            if (Inst == nullptr) return true;
        }
    }
    return false;// if it reaches here there was an instruction that could not be merged
}
void MergeB::updatePredecessorTerminator(llvm::BasicBlock* BToErase, llvm::BasicBlock* BToRetain) {
    llvm::SmallVector<llvm::BasicBlock*, 8> BBToUpdate(predecessors(BToErase));
    // get anywhere where BToErase is an operand and replace it with BToRetain
    for (llvm::BasicBlock* B : BBToUpdate) {
        llvm::Instruction* Term = B->getTerminator();
        unsigned int NumOpnds = Term->getNumOperands();
        for (unsigned int OpIdx = 0; OpIdx != NumOpnds; ++OpIdx) {
            if (Term->getOperand(OpIdx) == BToErase) {
                Term->setOperand(OpIdx, BToRetain);
            }
        }
    }
}
bool MergeB::canMergeInstructions(llvm::ArrayRef<llvm::Instruction*> Insts) {
    const llvm::Instruction* Inst1 = Insts[0];
    const llvm::Instruction* Inst2 = Insts[1];
    // instructions must be the same instruction type
    if (!Inst1->isSameOperationAs(Inst2)) return false;
    // must all have one use or all none
    bool HasUse = !Inst1->user_empty();
    for (llvm::Instruction* I : Insts) {
        if (HasUse && !I->hasOneUse())
            return false;
        if (!HasUse && !I->user_empty())
            return false;
    }
    // must not have a user but not be removable
    if (HasUse && !(canRemoveInst(Inst1) && canRemoveInst(Inst2)))
        return false;
    // must have same number of operands
    unsigned int NumOpnds = Inst1->getNumOperands();
    assert(NumOpnds == Inst2->getNumOperands());
    // operands must be equal
    for (unsigned int OpndIdx = 0; OpndIdx != NumOpnds; ++OpndIdx)
        if (Inst2->getOperand(OpndIdx) != Inst1->getOperand(OpndIdx))
            return false;
    return true;
}
bool MergeB::canRemoveInst(const llvm::Instruction* Inst) {
    assert(Inst->hasOneUse() && "Inst needs to have exactly one use");
    const llvm::PHINode* PNUse = llvm::dyn_cast<llvm::PHINode>(*Inst->user_begin());
    llvm::BasicBlock* Succ = Inst->getParent()->getTerminator()->getSuccessor(0);
    const llvm::Instruction* User = llvm::cast<llvm::Instruction>(*Inst->user_begin());
    // the instruction which uses the value must be in the same block as where the value is defined
    bool SameParentBB = User->getParent() == Inst->getParent();
    // or (single user must be a phi node and be in the successor block and contain the value as the predicate for its block)
    bool UsedInPhi = (PNUse != nullptr) && (PNUse->getParent() == Succ) && (PNUse->getIncomingValueForBlock(Inst->getParent()) == Inst);
    return UsedInPhi || SameParentBB;
}

llvm::PassPluginLibraryInfo getMergeBBPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "MergeB", LLVM_VERSION_STRING,
        [](llvm::PassBuilder& PB) {
            PB.registerPipelineParsingCallback(
                [](
                    llvm::StringRef Name, llvm::FunctionPassManager& FPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>
                ) {
                    if (Name == "merge-b") {
                        FPM.addPass(MergeB());
                        return true;
                    }
                    return false;
                }
            );
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getMergeBBPluginInfo();
}


unsigned int countNonDbgInstrInBB(llvm::BasicBlock* BB) {
    unsigned int Count = 0;
    for (llvm::Instruction& Instr : *BB)
        if (!llvm::isa<llvm::DbgInfoIntrinsic>(Instr))
            Count++;
    return Count;
}
llvm::Instruction* getLastNonDbgInst(llvm::BasicBlock* BB) {
    llvm::Instruction* Inst = BB->getTerminator();
    do {
        Inst = Inst->getPrevNode();
    } while (Inst && llvm::isa<llvm::DbgInfoIntrinsic>(Inst));
    return Inst;
}