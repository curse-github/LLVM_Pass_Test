#ifndef __DUPLICATE_B
#define __DUPLICATE_B
#ifdef _WIN32
    #ifdef _BUILD_TEMP
        #define TEMP_API __declspec(dllexport)
    #else
        #define TEMP_API __declspec(dllimport)
    #endif
#endif

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
struct TEMP_API Template : public llvm::PassInfoMixin<Template> {
    static bool isRequired() { return true; }
    llvm::PreservedAnalyses run(llvm::Function& F, llvm::FunctionAnalysisManager&);

    Template() = default;
    Template(const Template& copy) = delete;
    Template& operator=(const Template& copy) = delete;
    Template(Template&& move) = delete;
    Template& operator=(Template&& move) = delete;
    ~Template() = default;
};

#endif// __DUPLICATE_B