
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

    bool attemptMerge(llvm::BasicBlock* B, llvm::SmallPtrSet<llvm::BasicBlock*, 8> &DeleteList);
    bool canMergeBlocks(llvm::BasicBlock* B1, llvm::BasicBlock* B2, std::unordered_map<llvm::Value*, llvm::Value*>& PhiMap);
    bool canMergeInstructions(llvm::ArrayRef<llvm::Instruction*> Insts, std::unordered_map<llvm::Value*, llvm::Value*>& PhiMap);
    bool canRemoveInst(const llvm::Instruction* Inst);
    void updatePredecessorTerminator(llvm::BasicBlock* BToErase, llvm::BasicBlock* BToRetain);

    bool mergeUnconditionalConditionBranches(llvm::BasicBlock& B);
};

unsigned int countNonDbgInstrInB(llvm::BasicBlock* B);
llvm::Instruction* getLastNonDbgInst(llvm::BasicBlock* B);

#endif// __MERGE_B
