
#ifndef __MERGE_B
#define __MERGE_B

#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// inherits from llvm::PassInfoMixin for transfomation passes
// use llvm::InfoAnalysisMixin for analysis passes
struct MergeB : public llvm::PassInfoMixin<MergeB> {
    static bool isRequired() { return true; }

    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager&);
    bool attemptMerge(llvm::BasicBlock* BB, llvm::SmallPtrSet<llvm::BasicBlock*, 8> &DeleteList);
    bool canMergeBlocks(llvm::BasicBlock* B1, llvm::BasicBlock* B2);
    bool canMergeInstructions(llvm::ArrayRef<llvm::Instruction*> Insts);
    bool canRemoveInst(const llvm::Instruction* Inst);
    void updatePredecessorTerminator(llvm::BasicBlock* BToErase, llvm::BasicBlock* BToRetain);
};

unsigned int countNonDbgInstrInBB(llvm::BasicBlock* BB);
llvm::Instruction* getLastNonDbgInst(llvm::BasicBlock* BB);

class LockstepReverseIterator {
    llvm::BasicBlock* B1;
    llvm::BasicBlock* B2;
    llvm::SmallVector<llvm::Instruction*, 2> Insts;
    bool Fail;
public:
    LockstepReverseIterator(llvm::BasicBlock* B1In, llvm::BasicBlock* B2In);
    bool isValid() const { return !Fail; }
    void operator--();
    llvm::ArrayRef<llvm::Instruction* > operator*() const { return Insts; }
};

#endif// __MERGE_B
