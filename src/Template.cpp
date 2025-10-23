#define _BUILD_TEMP
#include "Template.h"

llvm::PreservedAnalyses Template::run(llvm::Function& F, llvm::FunctionAnalysisManager& FAM) {
    // this is the first function to be run, it gets run for each function in the file and determines the behaviour of the plugin
    return llvm::PreservedAnalyses::all();
    // instead return llvm::PreservedAnalyses::none() if you did change any code.
}
llvm::PassPluginLibraryInfo getTemplatePluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "Template", LLVM_VERSION_STRING,
        [](llvm::PassBuilder& passBuilder) {
            passBuilder.registerPipelineParsingCallback(
                [](
                    llvm::StringRef Name, llvm::FunctionPassManager& FPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>
                ) {
                    if (Name == "template") {
                        FPM.addPass(Template());
                        return true;
                    }
                    return false;
                }
            );
        }
    };
}
#pragma comment(linker, "/EXPORT:llvmGetPassPluginInfo")
extern "C"
llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getTemplatePluginInfo();
}