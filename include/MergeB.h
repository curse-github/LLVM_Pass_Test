
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
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager&);
    bool canRemoveInst(const llvm::Instruction* Inst);
    bool canMergeInstructions(llvm::ArrayRef<llvm::Instruction*> Insts);
    unsigned updateBranchTargets(llvm::BasicBlock* BBToErase, llvm::BasicBlock* BBToRetain);
    bool mergeDuplicatedBlock(llvm::BasicBlock* BB, llvm::SmallPtrSet<llvm::BasicBlock*, 8> &DeleteList);
    static bool isRequired() { return true; }
};

class LockstepReverseIterator {
    llvm::BasicBlock* BB1;
    llvm::BasicBlock* BB2;
    llvm::SmallVector<llvm::Instruction*, 2> Insts;
    bool Fail;
public:
    LockstepReverseIterator(llvm::BasicBlock* BB1In, llvm::BasicBlock* BB2In);
    llvm::Instruction* getLastNonDbgInst(llvm::BasicBlock* BB);
    bool isValid() const { return !Fail; }
    void operator--();
    llvm::ArrayRef<llvm::Instruction* > operator*() const { return Insts; }
};

#endif// __MERGE_B
