#include "MergeB.h"

#define DEBUG_TYPE "MergeB"
STATISTIC(NumDedupBBs, "Number of basic blocks merged");
STATISTIC(OverallNumOfUpdatedBranchTargets, "Number of updated branch targets");

bool MergeB::canRemoveInst(const llvm::Instruction* Inst) {
    assert(Inst->hasOneUse() && "Inst needs to have exactly one use");
    const llvm::PHINode* PNUse = llvm::dyn_cast<llvm::PHINode>(*Inst->user_begin());
    llvm::BasicBlock* Succ = Inst->getParent()->getTerminator()->getSuccessor(0);
    const llvm::Instruction* User = llvm::cast<llvm::Instruction>(*Inst->user_begin());
    bool SameParentBB = (User->getParent() == Inst->getParent());
    bool UsedInPhi = (PNUse && PNUse->getParent() == Succ && PNUse->getIncomingValueForBlock(Inst->getParent()) == Inst);
    return UsedInPhi || SameParentBB;
}
bool MergeB::canMergeInstructions(llvm::ArrayRef<llvm::Instruction* > Insts) {
    const llvm::Instruction* Inst1 = Insts[0];
    const llvm::Instruction* Inst2 = Insts[1];
    if (!Inst1->isSameOperationAs(Inst2))
        return false;
    bool HasUse = !Inst1->user_empty();
    for (llvm::Instruction* I : Insts) {
        if (HasUse && !I->hasOneUse())
            return false;
        if (!HasUse && !I->user_empty())
            return false;
    }
    if (HasUse && !(canRemoveInst(Inst1) && canRemoveInst(Inst2)))
        return false;
    assert(Inst2->getNumOperands() == Inst1->getNumOperands());
    unsigned int NumOpnds = Inst1->getNumOperands();
    for (unsigned int OpndIdx = 0; OpndIdx != NumOpnds; ++OpndIdx)
        if (Inst2->getOperand(OpndIdx) != Inst1->getOperand(OpndIdx))
            return false;
    return true;
}
static unsigned int getNumNonDbgInstrInBB(llvm::BasicBlock* BB) {
    unsigned int Count = 0;
    for (llvm::Instruction& Instr : *BB)
        if (!llvm::isa<llvm::DbgInfoIntrinsic>(Instr))
            Count++;
    return Count;
}
unsigned int MergeB::updateBranchTargets(llvm::BasicBlock* BBToErase, llvm::BasicBlock* BBToRetain) {
    llvm::SmallVector<llvm::BasicBlock*, 8> BBToUpdate(predecessors(BBToErase));
    LLVM_DEBUG(llvm::dbgs() << "DEDUP BB: merging duplicated blocks (" << BBToErase->getName() << " into " << BBToRetain->getName() << ")\n");
    unsigned int UpdatedTargetsCount = 0;
    for (llvm::BasicBlock* BB0 : BBToUpdate) {
        llvm::Instruction* Term = BB0->getTerminator();
        unsigned int NumOpnds = Term->getNumOperands();
        for (unsigned int OpIdx = 0; OpIdx != NumOpnds; ++OpIdx) {
            if (Term->getOperand(OpIdx) == BBToErase) {
                Term->setOperand(OpIdx, BBToRetain);
                UpdatedTargetsCount++;
            }
        }
    }
    return UpdatedTargetsCount;
}
bool MergeB::mergeDuplicatedBlock(llvm::BasicBlock* BB1, llvm::SmallPtrSet<llvm::BasicBlock*, 8>& DeleteList) {
    if (BB1 == &BB1->getParent()->getEntryBlock())
        return false;
    llvm::BranchInst* BB1Term = llvm::dyn_cast<llvm::BranchInst>(BB1->getTerminator());
    if (!(BB1Term && BB1Term->isUnconditional()))
        return false;
    for (llvm::BasicBlock* B : llvm::predecessors(BB1))
        if (!(llvm::isa<llvm::BranchInst>(B->getTerminator()) || llvm::isa<llvm::SwitchInst>(B->getTerminator())))
            return false;
    llvm::BasicBlock* BBSucc = BB1Term->getSuccessor(0);
    llvm::BasicBlock::iterator II = BBSucc->begin();
    const llvm::PHINode* PN = llvm::dyn_cast<llvm::PHINode>(II);
    llvm::Value* InValBB1 = nullptr;
    llvm::Instruction* InInstBB1 = nullptr;
    BBSucc->getFirstNonPHI();
    if (PN != nullptr) {
        if (++II != BBSucc->end() && llvm::isa<llvm::PHINode>(II))
            return false;
        InValBB1 = PN->getIncomingValueForBlock(BB1);
        InInstBB1 = llvm::dyn_cast<llvm::Instruction>(InValBB1);
    }
    unsigned int BB1NumInst = getNumNonDbgInstrInBB(BB1);
    for (llvm::BasicBlock* BB2 : predecessors(BBSucc)) {
        if (BB2 == &BB2->getParent()->getEntryBlock())
            continue;
        llvm::BranchInst* BB2Term = llvm::dyn_cast<llvm::BranchInst>(BB2->getTerminator());
        if (!(BB2Term && BB2Term->isUnconditional()))
            continue;
        for (llvm::BasicBlock* B : predecessors(BB2))
            if (!(llvm::isa<llvm::BranchInst>(B->getTerminator()) || llvm::isa<llvm::SwitchInst>(B->getTerminator())))
                continue;
        if (DeleteList.end() != DeleteList.find(BB2))
            continue;
        if (BB2 == BB1)
            continue;
        if (BB1NumInst != getNumNonDbgInstrInBB(BB2))
            continue;
        if (nullptr != PN) {
            llvm::Value* InValBB2 = PN->getIncomingValueForBlock(BB2);
            llvm::Instruction* InInstBB2 = llvm::dyn_cast<llvm::Instruction>(InValBB2);
            bool areValuesSimilar = (InValBB1 == InValBB2);
            bool bothValuesDefinedInParent = (InInstBB1 && InInstBB1->getParent() == BB1) && (InInstBB2 && InInstBB2->getParent() == BB2);
            if (!areValuesSimilar && !bothValuesDefinedInParent)
                continue;
        }
        LockstepReverseIterator LRI(BB1, BB2);
        while (LRI.isValid() && canMergeInstructions(*LRI))
            --LRI;
        if (LRI.isValid())
            continue;
        unsigned int UpdatedTargets = updateBranchTargets(BB1, BB2);
        assert(UpdatedTargets && "No branch target was updated");
        OverallNumOfUpdatedBranchTargets += UpdatedTargets;
        DeleteList.insert(BB1);
        NumDedupBBs++;
        return true;
    }
    return false;
}
llvm::PreservedAnalyses MergeB::run(llvm::Function& Func, llvm::FunctionAnalysisManager& ) {
    bool Changed = false;
    llvm::SmallPtrSet<llvm::BasicBlock*, 8> DeleteList;
    for (llvm::BasicBlock& BB : Func)
        Changed |= mergeDuplicatedBlock(&BB, DeleteList);
    for (llvm::BasicBlock* BB : DeleteList)
        DeleteDeadBlock(BB);
    return (Changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
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

LockstepReverseIterator::LockstepReverseIterator(llvm::BasicBlock* BB1In, llvm::BasicBlock* BB2In) : BB1(BB1In), BB2(BB2In), Fail(false) {
    Insts.clear();
    llvm::Instruction* InstBB1 = getLastNonDbgInst(BB1);
    if (nullptr == InstBB1)
        Fail = true;
    llvm::Instruction* InstBB2 = getLastNonDbgInst(BB2);
    if (nullptr == InstBB2)
        Fail = true;
    Insts.push_back(InstBB1);
    Insts.push_back(InstBB2);
}
llvm::Instruction* LockstepReverseIterator::getLastNonDbgInst(llvm::BasicBlock* BB) {
    llvm::Instruction* Inst = BB->getTerminator();
    do {
        Inst = Inst->getPrevNode();
    } while (Inst && llvm::isa<llvm::DbgInfoIntrinsic>(Inst));
    return Inst;
}
void LockstepReverseIterator::operator--() {
    if (Fail)
        return;
    for (llvm::Instruction*& Inst : Insts) {
        do {
            Inst = Inst->getPrevNode();
        } while (Inst && llvm::isa<llvm::DbgInfoIntrinsic>(Inst));
        if (!Inst) {
            Fail = true;
            return;
        }
    }
}