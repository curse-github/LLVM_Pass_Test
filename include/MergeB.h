
#ifndef __MERGE_B
#define __MERGE_B
#ifdef _WIN32
    #ifdef _BUILD_MER_B
        #define MER_B_API __declspec(dllexport)
    #else
        #define MER_B_API __declspec(dllimport)
    #endif
#else
    #define MER_B_API
#endif

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

struct MER_B_API MergeB : public llvm::PassInfoMixin<MergeB> {
    static bool isRequired() { return true; }
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager&);

    unsigned int deleteUselessBlock(llvm::Function& Func);
    bool hasPredecessors(llvm::BasicBlock* B);

    unsigned int attemptMerge(llvm::Function& Func);
    bool isMergeCandidate(llvm::BasicBlock* B);
    bool canMergeBlocks(llvm::BasicBlock* B1, llvm::BasicBlock* B2, std::unordered_map<llvm::PHINode*, llvm::Value*>& phiToInst);
    unsigned int countNonDbgInstrInB(llvm::BasicBlock* B);
    llvm::Instruction* getNextNonDebugInstruction(llvm::Instruction* Inst, llvm::Instruction* Term);
    bool canMergeInstructions(llvm::Instruction* Inst1, llvm::BasicBlock* B1, llvm::Instruction* Inst2, llvm::BasicBlock* B2, std::unordered_map<llvm::Value*, llvm::PHINode*>& instToPhi, std::unordered_map<llvm::PHINode*, llvm::Value*>& phiToInst);
    void updateBranchesToBlock(llvm::BasicBlock* BToErase, llvm::BasicBlock* BToRetain);

    unsigned int makeConditionalBranchesUnconditional(llvm::Function& Func);

    unsigned int mergeSinglePredecessorUnconditionalBranches(llvm::Function& F);

    unsigned int removeZeroUseInstructions(llvm::Function& F);
};

#endif// __MERGE_B
