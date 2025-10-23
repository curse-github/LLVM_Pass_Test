#define _BUILD_MER_B
#include "MergeB.h"

#include <iostream>

std::string toString(llvm::Instruction* Inst) {
    std::string str;
    llvm::raw_string_ostream rso(str);
    Inst->print(rso);
    return str;
}

llvm::PreservedAnalyses MergeB::run(llvm::Function& Func, llvm::FunctionAnalysisManager& ) {
    unsigned int TotalChanged = 0;
    unsigned int JustChanged = 0;
    do {
        JustChanged = attemptMerge(Func);
        JustChanged += deleteUselessBlock(Func);
        JustChanged += makeConditionalBranchesUnconditional(Func);
        JustChanged += mergeSinglePredecessorUnconditionalBranches(Func);
        JustChanged += deleteUselessBlock(Func);
        JustChanged += removeZeroUseInstructions(Func);
        TotalChanged += JustChanged;
    } while (JustChanged > 0);
    return ((TotalChanged > 0) ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
}


unsigned int MergeB::deleteUselessBlock(llvm::Function& Func) {
    // find dead blocks and delete them
    std::vector<llvm::BasicBlock*> toDelete{};
    llvm::BasicBlock* eB = &Func.getEntryBlock();
    for(llvm::BasicBlock& B : Func)
        if (!hasPredecessors(&B) && (&B != eB))
            toDelete.push_back(&B);
    llvm::DeleteDeadBlocks(toDelete);
    return static_cast<unsigned int>(toDelete.size());
}
bool MergeB::hasPredecessors(llvm::BasicBlock* B) {
    llvm::pred_range tmp = llvm::predecessors(B);
    return tmp.begin() != tmp.end();
}


unsigned int MergeB::attemptMerge(llvm::Function& Func) {
    unsigned int LocalChanged = false;
    llvm::Function::iterator bE = Func.end();
    for (llvm::Function::iterator bI = Func.begin(); bI != bE; ++bI) {
        llvm::BasicBlock* B1 = &*bI;
        if (!isMergeCandidate(B1)) continue;
        // search through the other predecessors of this blocks successor to find B2
        for (llvm::BasicBlock* B2 : llvm::predecessors(B1->getSingleSuccessor())) {
            // check if all the two blocks can be merged
            std::unordered_map<llvm::PHINode*, llvm::Value*> phiToInst{};
            if (!canMergeBlocks(B1, B2, phiToInst)) continue;
            // replace all uses of the phi instructions with the 
            for (std::pair<llvm::PHINode*, llvm::Value*> PnVP : phiToInst) {
                std::string str = PnVP.first->getName().str();
                PnVP.first->setName(str + ".old");
                PnVP.second->setName(str);
                while (PnVP.first->getNumUses() != 0)
                    PnVP.first->use_begin()->set(PnVP.second);
            }
            // update anywhere branching to B2 and replace it with B1
            updateBranchesToBlock(B2, B1);
            LocalChanged++;
        }
    }
    return LocalChanged;
}
bool MergeB::isMergeCandidate(llvm::BasicBlock* B) {
    // cannot be unconditional branch
    llvm::BranchInst* BTerm = llvm::dyn_cast<llvm::BranchInst>(B->getTerminator());
    if ((BTerm == nullptr) || !BTerm->isUnconditional()) return false;
    // predecessor must be either branch or switch
    for (llvm::BasicBlock* Pred : llvm::predecessors(B))
        if (!llvm::isa<llvm::BranchInst>(Pred->getTerminator()))
            return false;
    // must have only one predecessor
    llvm::BasicBlock* B1Pred = B->getSinglePredecessor();
    if (B1Pred == nullptr) return false;
    // and only one successor
    llvm::BasicBlock* B1Succ = B->getSingleSuccessor();
    if (B1Succ == nullptr) return false;
    return true;
}
bool MergeB::canMergeBlocks(llvm::BasicBlock* B1, llvm::BasicBlock* B2, std::unordered_map<llvm::PHINode*, llvm::Value*>& phiToInst) {
    // terminator must be unconditional
    llvm::BranchInst* B2Term = llvm::dyn_cast<llvm::BranchInst>(B2->getTerminator());
    if ((B2Term == nullptr) || !B2Term->isUnconditional()) return false;
    // cannot be equal to B1
    if (B1 == B2) return false;
    // must have same sucessor and predecessor
    if (B1->getSinglePredecessor() != B2->getSinglePredecessor()) return false;
    if (B2->getSinglePredecessor() != B2->getSinglePredecessor()) return false;
    // must have the same number of instructions as B1 (just a quick check before actually checking all instructions)
    if (countNonDbgInstrInB(B1) != countNonDbgInstrInB(B2)) return false;
    // get first non debug instructions
    llvm::Instruction* B1Term = B1->getTerminator();
    llvm::Instruction* InstB1 = &*B1->begin();
    if (llvm::isa<llvm::DbgInfoIntrinsic>(InstB1))
        InstB1 = getNextNonDebugInstruction(InstB1, B1Term);
    if (InstB1 == B1Term) return true;

    llvm::Instruction* InstB2 = &*B2->begin();
    if (llvm::isa<llvm::DbgInfoIntrinsic>(InstB2))
        InstB2 = getNextNonDebugInstruction(InstB2, B2Term);
    if (InstB2 == B2Term) return true;
    // go through each instructions and check if they can be merged
    std::unordered_map<llvm::Value*, llvm::PHINode*> instToPhi{};
    while (canMergeInstructions(InstB1, B1, InstB2, B2, instToPhi, phiToInst)) {
        InstB1 = getNextNonDebugInstruction(InstB1, B1Term);
        if (InstB1 == B1Term) return true;
        InstB2 = getNextNonDebugInstruction(InstB2, B2Term);
        if (InstB2 == B2Term) return true;
    }
    return false;// if it reaches here there was an instruction that could not be merged
}
unsigned int MergeB::countNonDbgInstrInB(llvm::BasicBlock* B) {
    unsigned int Count = 0;
    for (llvm::Instruction& Inst : *B)
        if (!llvm::isa<llvm::DbgInfoIntrinsic>(Inst))
            Count++;
    return Count;
}
llvm::Instruction* MergeB::getNextNonDebugInstruction(llvm::Instruction* Inst, llvm::Instruction* Term) {
    do {
        Inst = Inst->getNextNode();
    } while ((Inst != Term) && llvm::isa<llvm::DbgInfoIntrinsic>(Inst));
    return Inst;
}
bool MergeB::canMergeInstructions(llvm::Instruction* Inst1, llvm::BasicBlock* B1, llvm::Instruction* Inst2, llvm::BasicBlock* B2, std::unordered_map<llvm::Value*, llvm::PHINode*>& instToPhi, std::unordered_map<llvm::PHINode*, llvm::Value*>& phiToInst) {
    // instructions must be the same instruction type
    if (!Inst1->isSameOperationAs(Inst2))
        return false;
    // and cant be phi instruction.
    if (llvm::isa<llvm::PHINode>(Inst1))
        return false;
    // should be used the same number of times
    unsigned int numUses = Inst1->getNumUses();
    if (numUses != Inst2->getNumUses())
        return false;
    // must have same number of operands
    unsigned int NumOpnds = Inst1->getNumOperands();
    if (NumOpnds != Inst2->getNumOperands())
        return false;
    // operands must be equal (or will be equal after fixing the phi instructions)
    for (unsigned int OpndIdx = 0; OpndIdx != NumOpnds; ++OpndIdx)
        if (
            (Inst1->getOperand(OpndIdx) != Inst2->getOperand(OpndIdx)) &&
            (instToPhi[Inst1->getOperand(OpndIdx)] != instToPhi[Inst2->getOperand(OpndIdx)])
        )
            return false;
    // add to phi map
    if (Inst1->getType()->isVoidTy()) return true;
    llvm::BasicBlock* B1Succ = B1->getSingleSuccessor();
    llvm::BasicBlock::iterator bE = B1Succ->end();
    for (llvm::BasicBlock::iterator bI = B1Succ->begin(); bI != bE; ++bI) {
        llvm::PHINode* PN = llvm::dyn_cast<llvm::PHINode>(&*bI);
        if (PN == nullptr) break;
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
void MergeB::updateBranchesToBlock(llvm::BasicBlock* BToErase, llvm::BasicBlock* BToRetain) {
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


unsigned int MergeB::makeConditionalBranchesUnconditional(llvm::Function& Func) {
    unsigned int LocalChanged = false;
    // loop through function
    llvm::Function::iterator bE = Func.end();
    for (llvm::Function::iterator bI = Func.begin(); bI != bE; ++bI) {
        llvm::BasicBlock* B = &*bI;
        // if is conditional branch
        llvm::BranchInst* br = llvm::dyn_cast<llvm::BranchInst>(B->getTerminator());
        if (br == nullptr) continue;
        if (br->isUnconditional()) continue;
        // but both condition blocks are equal
        llvm::Value* two = br->getOperand(1);
        llvm::Value* three = br->getOperand(2);
        if (two != three) continue;
        // create a new unconditional branch instruction to that block
        llvm::BasicBlock* label = llvm::dyn_cast<llvm::BasicBlock>(two);
        llvm::BranchInst* newBr = llvm::BranchInst::Create(label);
        // replace the original instruction
        llvm::ReplaceInstWithInst(B, --B->end(), newBr);
        LocalChanged++;
    }
    return LocalChanged;
}


unsigned int MergeB::mergeSinglePredecessorUnconditionalBranches(llvm::Function& Func) {
    unsigned int LocalChanged = false;
    llvm::Function::iterator bE = Func.end();
    for (llvm::Function::iterator bI = Func.begin(); bI != bE; ++bI) {
        llvm::BasicBlock* B = &*bI;
        // must have since successor
        llvm::BasicBlock* label = B->getSingleSuccessor();
        if (label == nullptr) continue;
        // predecessor must be the current block
        if (label->getSinglePredecessor() != B) continue;
        // remove original terminator, will be replaced with one from the successor block
        B->getTerminator()->eraseFromParent();
        // loop through instructions in successor and put them into the predecessor block
        llvm::BasicBlock::iterator lE = label->end();
        for(llvm::BasicBlock::iterator lI = label->begin(); lI != lE; ++lI) {
            llvm::Instruction* Inst = &*lI;
            // clone instruction, and put it into the predecessor block
            llvm::Instruction* InstClone = Inst->clone();
            InstClone->insertBefore(B->end());
            // fix names
            std::string name = Inst->getName().str();
            Inst->setName(name+".old");
            InstClone->setName(name);
            // update uses of that original value to the new value
            while (Inst->getNumUses() != 0)
                Inst->use_begin()->set(InstClone);
        }
        // update any phi nodes to use new merged block as incoming block instead of old block
        for(llvm::BasicBlock* Succ : llvm::successors(label)) {
            llvm::BasicBlock::iterator E = Succ->end();
            for(llvm::BasicBlock::iterator I = Succ->begin(); I != E; ++I) {
                llvm::PHINode* PN = llvm::dyn_cast<llvm::PHINode>(&*I);
                if (PN == nullptr) break;
                PN->addIncoming(PN->getIncomingValueForBlock(label), B);
            }
        }
        LocalChanged++;
    }
    return LocalChanged;
}


unsigned int MergeB::removeZeroUseInstructions(llvm::Function& Func) {
    std::vector<llvm::Instruction*> toDelete{};
    // loop through function
    llvm::Function::iterator bE = Func.end();
    for (llvm::Function::iterator bI = Func.begin(); bI != bE; ++bI) {
        llvm::BasicBlock* B = &*bI;
        // loop through block
        llvm::BasicBlock::iterator iE = B->end();
        for (llvm::BasicBlock::iterator iI = B->begin(); iI != iE; ++iI) {
            llvm::Instruction* Inst = &*iI;
            // if instruction has no use mark it to be deleted
            if ((Inst->getNumUses() == 0) && !Inst->isTerminator() && !llvm::isa<llvm::CallInst>(Inst) && !llvm::isa<llvm::StoreInst>(Inst))
                toDelete.push_back(Inst);
        }
    }
    // actually delete the un-used instructions.
    for (llvm::Instruction* Inst : toDelete)
        Inst->eraseFromParent();
    return static_cast<unsigned int>(toDelete.size());
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
#ifdef _WIN32
    #pragma comment(linker, "/EXPORT:llvmGetPassPluginInfo")
#endif
extern "C"
llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getMergeBPluginInfo();
}