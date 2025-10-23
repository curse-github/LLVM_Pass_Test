#ifndef LLVM_TUTOR_RIV_H
#define LLVM_TUTOR_RIV_H
#ifdef _WIN32
    #ifdef _BUILD_RIV
        #define RIV_API __declspec(dllexport)
    #else
        #define RIV_API __declspec(dllimport)
    #endif
#else
    #define RIV_API
#endif

#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassPlugin.h"

struct RIV : public llvm::AnalysisInfoMixin<RIV> {
    using Result = llvm::MapVector<const llvm::BasicBlock*, llvm::SmallPtrSet<llvm::Value*, 8>>;
    Result run(llvm::Function& F, llvm::FunctionAnalysisManager&);
    Result buildRIV(llvm::Function& F, llvm::DomTreeNodeBase<llvm::BasicBlock>* CFGRoot);
private:
    RIV_API
    static llvm::AnalysisKey Key;
    friend struct llvm::AnalysisInfoMixin<RIV>;
};

class RIVPrinter : public llvm::PassInfoMixin<RIVPrinter> {
public:
    explicit RIVPrinter(llvm::raw_ostream& OutS) : OS(OutS) {}
    llvm::PreservedAnalyses run(llvm::Function& F, llvm::FunctionAnalysisManager& FAM);
private:
    llvm::raw_ostream& OS;
};

#endif // LLVM_TUTOR_RIV_H
