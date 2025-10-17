#ifndef __DUPLICATE_B
#define __DUPLICATE_B

#include "RIV.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <map>
#include <memory>
#include <random>

// inherits from llvm::PassInfoMixin for transfomation passes
// use llvm::InfoAnalysisMixin for analysis passes
struct DuplicateB : public llvm::PassInfoMixin<DuplicateB> {
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &);
    std::vector<std::tuple<llvm::BasicBlock *, llvm::Value *>> findBBsToDuplicate(llvm::Function &F, const RIV::Result &RIVResult);
    void cloneBB(llvm::BasicBlock &BB, llvm::Value *ContextValue, std::map<llvm::Value *, llvm::Value *> &ReMapper);
    unsigned DuplicateBBCount = 0;
    static bool isRequired() { return true; }
    std::unique_ptr<llvm::RandomNumberGenerator> pRNG;
};

#endif// __DUPLICATE_B