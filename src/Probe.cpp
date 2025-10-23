#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
using namespace llvm;

struct ProbeDT : public PassInfoMixin<ProbeDT> {
    PreservedAnalyses run(Function& F, FunctionAnalysisManager& FAM) {
        errs() << "[probe-dt] function: " << F.getName() << "\n";
        DominatorTree& DT = FAM.getResult<DominatorTreeAnalysis>(F);
        llvm::DomTreeNodeBase<llvm::BasicBlock>* Root = DT.getRootNode();
        if (!Root) {
            errs() << "[probe-dt] no root (CFG weird?)\n";
            return PreservedAnalyses::all();
        }
        size_t count = 0;
        for (auto I = df_begin(Root), E = df_end(Root); I != E; ++I)
            ++count;
        errs() << "[probe-dt] nodes: " << count << "\n";
        return PreservedAnalyses::all();
    }
};

// New-PM plugin boilerplate
static PassPluginLibraryInfo getProbeInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "probe-dt", LLVM_VERSION_STRING,
        [](PassBuilder& passBuilder) {
            passBuilder.registerAnalysisRegistrationCallback([](
                FunctionAnalysisManager &FAM
            ) {
                llvm::errs() << "Registering DominatorTreeAnalysis and LoopAnalysis\n";
                FAM.registerPass([]() { return DominatorTreeAnalysis(); });
            });
            passBuilder.registerPipelineParsingCallback([](
                StringRef Name, FunctionPassManager &FPM,
                ArrayRef<PassBuilder::PipelineElement>
            ) {
                if (Name == "probe-dt") {
                    FPM.addPass(ProbeDT());
                    return true;
                }
                return false;
            });
        }
    };
}
#ifdef _WIN32
    #pragma comment(linker, "/EXPORT:llvmGetPassPluginInfo")
#endif
extern "C" llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getProbeInfo();
}