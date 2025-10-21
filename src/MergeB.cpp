#include "MergeB.h"

#include <iostream>

unsigned int removeDeadBlocks(llvm::Function& Func) {
    // find dead blocks and delete them
    std::vector<llvm::BasicBlock*> toDelete{};
    llvm::BasicBlock* eB = &Func.getEntryBlock();
    for(llvm::BasicBlock& B : Func)
        if (!hasPredecessors(&B) && (&B != eB))
            toDelete.push_back(&B);
    llvm::DeleteDeadBlocks(toDelete);
    return static_cast<unsigned int>(toDelete.size());
}
llvm::PreservedAnalyses MergeB::run(llvm::Function& Func, llvm::FunctionAnalysisManager& ) {
    if (Func.getName() != "main") return llvm::PreservedAnalyses::all();
    unsigned int TotalChanged = 0;
    unsigned int JustChanged1 = 0;
    unsigned int JustChanged2 = 0;
    do {
        JustChanged1 = 0;
        for (llvm::BasicBlock& B : Func)
            JustChanged1 += attemptMerge(&B);
        JustChanged1 += removeDeadBlocks(Func);
        for (llvm::BasicBlock& B : Func)
            JustChanged1 += makeConditionalBranchesUnconditional(B);
        do {
            JustChanged2 = mergeSinglePredecessorUnconditionalBranches(Func);
            JustChanged2 += removeDeadBlocks(Func);
            JustChanged1 += JustChanged2;
        } while (JustChanged2 > 0);
        TotalChanged += JustChanged1;
    } while (JustChanged1 > 0);
    return ((TotalChanged > 0) ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
}

std::string toString(llvm::Instruction* Inst) {
    std::string str;
    llvm::raw_string_ostream rso(str);
    Inst->print(rso);
    return str;
}
unsigned int MergeB::attemptMerge(llvm::BasicBlock* B1) {
    // cannot be unconditional branch
    llvm::BranchInst* B1Term = llvm::dyn_cast<llvm::BranchInst>(B1->getTerminator());
    if ((B1Term == nullptr) || !B1Term->isUnconditional()) return 0;
    // predecessor must be either branch or switch
    for (llvm::BasicBlock* B : llvm::predecessors(B1))
        if (!(llvm::isa<llvm::BranchInst>(B->getTerminator()) || llvm::isa<llvm::SwitchInst>(B->getTerminator())))
            return 0;
    // must have only one predecessor and successor
    llvm::BasicBlock* B1Pred = B1->getSinglePredecessor();
    if (B1Pred == nullptr) return 0;
    llvm::BasicBlock* B1Succ = B1->getSingleSuccessor();
    if (B1Succ == nullptr) return 0;
    // defined up here so it can be reused
    unsigned int B1NumInst = countNonDbgInstrInB(B1);
    // get first instruction of sucessor block
    llvm::BasicBlock* BSucc = B1Term->getSuccessor(0);
    for (llvm::BasicBlock* B2 : predecessors(BSucc)) {
        // terminator must be unconditional
        llvm::BranchInst* B2Term = llvm::dyn_cast<llvm::BranchInst>(B2->getTerminator());
        if ((B2Term == nullptr) || !B2Term->isUnconditional()) continue;
        // cannot be equal to B1
        if (B1 == B2) continue;
        // must have same sucessor and predecessor
        llvm::BasicBlock* B2Pred = B2->getSinglePredecessor();
        if (B2Pred != B1Pred) continue;
        llvm::BasicBlock* B2Succ = B2->getSingleSuccessor();
        if (B2Succ != B1Succ) continue;
        // must have the same number of instructions as B1 (just a quick check before actually checking all instructions)
        if (B1NumInst != countNonDbgInstrInB(B2)) continue;
        // check if all instructions are mergable with one another in both blocks (see canMergeBlocks)
        std::unordered_map<llvm::PHINode*, llvm::Value*> phiToInst{};
        if (!canMergeBlocks(B1, B2, phiToInst)) continue;
        for (std::pair<llvm::PHINode*, llvm::Value*> PnVP : phiToInst) {
            while (PnVP.first->getNumUses() != 0)
                PnVP.first->use_begin()->set(PnVP.second);
            // PnVP.first->removeFromParent();
        }
        // update anywhere branching to B2 and replace it with B1
        updatePredecessorTerminator(B2, B1);
        return 1;
    }
    return 0;
}
bool MergeB::canMergeBlocks(llvm::BasicBlock* B1, llvm::BasicBlock* B2, std::unordered_map<llvm::PHINode*, llvm::Value*>& phiToInst) {
    llvm::Instruction* InstB1 = &*B1->begin();
    llvm::Instruction* B1Term = B1->getTerminator();
    while ((InstB1 != B1Term) && llvm::isa<llvm::DbgInfoIntrinsic>(InstB1))
        InstB1 = InstB1->getNextNode();
    if (InstB1 == B1Term) return true;
    llvm::Instruction* InstB2 = &*B2->begin();
    llvm::Instruction* B2Term = B2->getTerminator();
    while ((InstB2 != B2Term) && llvm::isa<llvm::DbgInfoIntrinsic>(InstB2))
        InstB2 = InstB2->getNextNode();
    if (InstB2 == B2Term) return true;
    // get previous not untill it reaches the beginning nad check for if you are able to merge each instruction
    std::unordered_map<llvm::Value*, llvm::PHINode*> instToPhi{};
    while (canMergeInstructions(InstB1, B1, InstB2, B2, instToPhi, phiToInst)) {
        do
            InstB1 = InstB1->getNextNode();
        while ((InstB1 != B1Term) && llvm::isa<llvm::DbgInfoIntrinsic>(InstB1));
        if (InstB1 == B1Term) return true;
        do
            InstB2 = InstB2->getNextNode();
        while ((InstB2 == B2Term) && llvm::isa<llvm::DbgInfoIntrinsic>(InstB2));
        if (InstB2 == B2Term) return true;
    }
    return false;// if it reaches here there was an instruction that could not be merged
}
bool MergeB::canMergeInstructions(llvm::Instruction* Inst1, llvm::BasicBlock* B1, llvm::Instruction* Inst2, llvm::BasicBlock* B2, std::unordered_map<llvm::Value*, llvm::PHINode*>& instToPhi, std::unordered_map<llvm::PHINode*, llvm::Value*>& phiToInst) {
// instructions must be the same instruction type
    if (!Inst1->isSameOperationAs(Inst2))
        return false;
    if (llvm::isa<llvm::PHINode>(Inst1))
        return false;
    // must all have one use or all none
    unsigned int numUses = Inst1->getNumUses();
    // should be used the same number of times
    if (numUses != Inst2->getNumUses())
        return false;
    // must have same number of operands
    unsigned int NumOpnds = Inst1->getNumOperands();
    if (NumOpnds != Inst2->getNumOperands())
        return false;
    // operands must be equal
    for (unsigned int OpndIdx = 0; OpndIdx != NumOpnds; ++OpndIdx)
        if (
            (Inst1->getOperand(OpndIdx) != Inst2->getOperand(OpndIdx)) &&
            (instToPhi[Inst1->getOperand(OpndIdx)] != instToPhi[Inst2->getOperand(OpndIdx)])
        )
            return false;
    // add to phi map
    if (Inst1->getType()->isVoidTy()) return true;
    llvm::PHINode* PN = nullptr;
    llvm::Value::user_iterator uE = Inst1->user_end();
    llvm::BasicBlock* BSucc = B1->getTerminator()->getSuccessor(0);
    for (llvm::Value::user_iterator uI = Inst1->user_begin(); uI != uE; ++uI) {
        PN = llvm::dyn_cast<llvm::PHINode>(*uI);
        if (PN == nullptr) continue;
        // must be within the shared successor of B1 and B2
        if (PN->getParent() != BSucc) { PN = nullptr; continue; }
        // must contain predicated for B1 and B2 which are the values Inst1 and Inst2 respectively
        if (PN->getIncomingValueForBlock(B1) != Inst1) { PN = nullptr; continue; }
        if (PN->getIncomingValueForBlock(B2) != Inst2) { PN = nullptr; continue; }
        // add to map
        instToPhi[Inst1] = PN;
        instToPhi[Inst2] = PN;
        phiToInst[PN] = Inst1;
    }
    return true;
}
unsigned int countNonDbgInstrInB(llvm::BasicBlock* B) {
    unsigned int Count = 0;
    for (llvm::Instruction& Instr : *B)
        if (!llvm::isa<llvm::DbgInfoIntrinsic>(Instr))
            Count++;
    return Count;
}
void MergeB::updatePredecessorTerminator(llvm::BasicBlock* BToErase, llvm::BasicBlock* BToRetain) {
    // get anywhere where BToErase is an operand and replace it with BToRetain
    llvm::SmallVector<llvm::BasicBlock*, 8> BToUpdate(predecessors(BToErase));
    for (llvm::BasicBlock* B : BToUpdate) {
        llvm::Instruction* Term = B->getTerminator();
        unsigned int NumOpnds = Term->getNumOperands();
        for (unsigned int OpIdx = 0; OpIdx != NumOpnds; ++OpIdx)
            if (Term->getOperand(OpIdx) == BToErase)
                Term->setOperand(OpIdx, BToRetain);
    }
}
bool hasPredecessors(llvm::BasicBlock* B) {
    llvm::pred_range tmp = llvm::predecessors(B);
    return tmp.begin() != tmp.end();
}


unsigned int MergeB::makeConditionalBranchesUnconditional(llvm::BasicBlock& B) {
    unsigned int Changed = false;
    llvm::BasicBlock::iterator bE = B.end();
    for (llvm::BasicBlock::iterator bI = B.begin(); bI != bE; ++bI) {
        llvm::BranchInst* br = llvm::dyn_cast<llvm::BranchInst>(&*bI);
        if (br == nullptr) continue;
        if (br->isUnconditional()) continue;
        llvm::Value* two = br->getOperand(1);
        llvm::Value* three = br->getOperand(2);
        if (two != three) continue;
        llvm::BasicBlock* label = llvm::dyn_cast<llvm::BasicBlock>(two);
        if (label == nullptr) continue;
        llvm::BranchInst* newBr = llvm::BranchInst::Create(label);
        llvm::ReplaceInstWithInst(&B, bI, newBr);
        Changed++;
    }
    return Changed;
}


unsigned int MergeB::mergeSinglePredecessorUnconditionalBranches(llvm::Function& F) {
    unsigned int LocalChanged = false;
    llvm::Function::iterator bE = F.end();
    bool doIncrement = true;
    for (llvm::Function::iterator bI = F.begin(); bI != bE; doIncrement?(++bI):bI) {
        //if (LocalChanged>3) break;
        doIncrement = true;
        llvm::BasicBlock* B = &*bI;
        if (!hasPredecessors(B) && (B != &F.getEntryBlock())) continue;
        llvm::BasicBlock* label = B->getSingleSuccessor();
        if (label == nullptr) continue;
        // predecessor must be the current block or one we already deleted
        if (label->getSinglePredecessor() != B) continue;
        // remove original terminator, will be replaced with one from the successor block
        B->getTerminator()->eraseFromParent();
        // loop through instructions in successor and put them into the predecessor block
        llvm::BasicBlock::iterator lE = label->end();
        for(llvm::BasicBlock::iterator lI = label->begin(); lI != lE; ++lI) {
            llvm::Instruction* Inst = &*lI;
            // clone instruction, re-map it, and put it into the predecessor block
            llvm::Instruction* InstClone = Inst->clone();
            InstClone->insertBefore(B->end());
            // update uses of that value
            while (Inst->getNumUses() != 0)
                Inst->use_begin()->set(InstClone);
        }
        for(llvm::BasicBlock* Succ : llvm::successors(label)) {
            llvm::BasicBlock::iterator E = Succ->end();
            for(llvm::BasicBlock::iterator I = Succ->begin(); I != E; ++I) {
                llvm::PHINode* PN = llvm::dyn_cast<llvm::PHINode>(&*I);
                if (PN == nullptr) break;
                PN->addIncoming(PN->getIncomingValueForBlock(label), B);
            }
        }
        doIncrement = false;
        LocalChanged++;
    }
    return LocalChanged;
}


unsigned int MergeB::removeNoUseInstructions(llvm::Function& F) {
    return 0;
}


llvm::PassPluginLibraryInfo getMergeBPluginInfo() {
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
    return getMergeBPluginInfo();
}