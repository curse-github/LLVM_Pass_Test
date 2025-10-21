#ifndef __DUPLICATE_B
#define __DUPLICATE_B

#include "RIV.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <memory>
#include <random>
#include <vector>

struct DuplicateB : public llvm::PassInfoMixin<DuplicateB> {
    std::unique_ptr<llvm::RandomNumberGenerator> RngGen;
    static bool isRequired() { return true; }

    llvm::PreservedAnalyses run(llvm::Function& F, llvm::FunctionAnalysisManager&);
    std::vector<std::pair<llvm::BasicBlock*, llvm::Value*>> findDuplicatableBs(llvm::Function& F, const RIV::Result& RIVResult);
    void duplicateB(llvm::BasicBlock* B, llvm::Value* TargetVal, std::unordered_map<llvm::Value*, llvm::Value*>& ValRe_mapper);
    unsigned DuplicateBBCount = 0;
};

#endif// __DUPLICATE_B