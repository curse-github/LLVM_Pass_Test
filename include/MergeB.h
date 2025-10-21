
#ifndef __MERGE_B
#define __MERGE_B

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

struct MergeB : public llvm::PassInfoMixin<MergeB> {
    static bool isRequired() { return true; }

    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager&);

    unsigned int attemptMerge(llvm::BasicBlock* B);
    bool canMergeBlocks(llvm::BasicBlock* B1, llvm::BasicBlock* B2, std::unordered_map<llvm::PHINode*, llvm::Value*>& phiToInst);
    bool canMergeInstructions(llvm::Instruction* Inst1, llvm::BasicBlock* B1, llvm::Instruction* Inst2, llvm::BasicBlock* B2, std::unordered_map<llvm::Value*, llvm::PHINode*>& instToPhi, std::unordered_map<llvm::PHINode*, llvm::Value*>& phiToInst);
    void updatePredecessorTerminator(llvm::BasicBlock* BToErase, llvm::BasicBlock* BToRetain);

    unsigned int makeConditionalBranchesUnconditional(llvm::BasicBlock& B);

    unsigned int mergeSinglePredecessorUnconditionalBranches(llvm::Function& F);

    unsigned int removeZeroUseInstructions(llvm::Function& F);
};

unsigned int countNonDbgInstrInB(llvm::BasicBlock* B);
bool hasPredecessors(llvm::BasicBlock* B);

#endif// __MERGE_B
