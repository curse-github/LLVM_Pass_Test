#include "DuplicateB.h"

#define DEBUG_TYPE "duplicate-bb"
STATISTIC(DuplicateBBCountStats, "The # of duplicated blocks");

std::vector<std::tuple<llvm::BasicBlock *, llvm::Value *>>
DuplicateB::findBBsToDuplicate(llvm::Function& F, const RIV::Result& RIVResult) {
    std::vector<std::tuple<llvm::BasicBlock *, llvm::Value *>> BlocksToDuplicate;
    for (llvm::BasicBlock& BB : F) {
        if (BB.isLandingPad())
            continue;
        const llvm::SmallPtrSet<llvm::Value *, 8U>& ReachableValues = RIVResult.lookup(&BB);
        size_t ReachableValuesCount = ReachableValues.size();
        if (0 == ReachableValuesCount) {
            LLVM_DEBUG(llvm::errs() << "No context values for this BB\n");
            continue;
        }
        llvm::SmallPtrSetIterator<llvm::Value *> Iter = ReachableValues.begin();
        std::uniform_int_distribution<> Dist(0, ReachableValuesCount - 1);
        std::advance(Iter, Dist(*pRNG));
        if (llvm::dyn_cast<llvm::GlobalValue>(*Iter)) {
            LLVM_DEBUG(llvm::errs() << "Random context value is a global variable. " << "Skipping this BB\n");
            continue;
        }
        LLVM_DEBUG(llvm::errs() << "Random context value: " <<* *Iter << "\n");
        BlocksToDuplicate.emplace_back(&BB,* Iter);
    }
    return BlocksToDuplicate;
}
void DuplicateB::cloneBB(llvm::BasicBlock& BB, llvm::Value* ContextValue, std::map<llvm::Value*, llvm::Value *>& ReMapper) {
    llvm::BasicBlock::iterator BBHead = BB.getFirstNonPHIIt();
    llvm::IRBuilder<> Builder(&*BBHead);
    llvm::Value* Cond = Builder.CreateIsNull(ReMapper.count(ContextValue) ? ReMapper[ContextValue] : ContextValue);
    llvm::Instruction* ThenTerm = nullptr;
    llvm::Instruction* ElseTerm = nullptr;
    SplitBlockAndInsertIfThenElse(Cond, &*BBHead, &ThenTerm, &ElseTerm);
    llvm::BasicBlock* Tail = ThenTerm->getSuccessor(0);
    assert(Tail == ElseTerm->getSuccessor(0) && "Inconsistent CFG");
    std::string DuplicatedBBId = std::to_string(DuplicateBBCount);
    ThenTerm->getParent()->setName("lt-clone-1-" + DuplicatedBBId);
    ElseTerm->getParent()->setName("lt-clone-2-" + DuplicatedBBId);
    Tail->setName("lt-tail-" + DuplicatedBBId);
    ThenTerm->getParent()->getSinglePredecessor()->setName("lt-if-then-else-" + DuplicatedBBId);
    llvm::ValueToValueMapTy TailVMap, ThenVMap, ElseVMap;
    llvm::SmallVector<llvm::Instruction*, 8> ToRemove;
    for (llvm::BasicBlock::iterator IIT = Tail->begin(), IE = Tail->end(); IIT != IE; ++IIT) {
        llvm::Instruction* Instr = &*IIT;
        assert(!llvm::isa<llvm::PHINode>(Instr) && "Phi nodes have already been filtered out");
        if (Instr->isTerminator()) {
            RemapInstruction(Instr, TailVMap, llvm::RF_IgnoreMissingLocals);
            continue;
        }
        llvm::Instruction* ThenClone = Instr->clone();
        llvm::Instruction* ElseClone = Instr->clone();
        RemapInstruction(ThenClone, ThenVMap, llvm::RF_IgnoreMissingLocals);
        ThenClone->insertBefore(ThenTerm->getIterator());
        ThenVMap[Instr] = ThenClone;
        RemapInstruction(ElseClone, ElseVMap, llvm::RF_IgnoreMissingLocals);
        ElseClone->insertBefore(ElseTerm->getIterator());
        ElseVMap[Instr] = ElseClone;
        if (ThenClone->getType()->isVoidTy()) {
            ToRemove.push_back(Instr);
            continue;
        }
        llvm::PHINode* Phi = llvm::PHINode::Create(ThenClone->getType(), 2);
        Phi->addIncoming(ThenClone, ThenTerm->getParent());
        Phi->addIncoming(ElseClone, ElseTerm->getParent());
        TailVMap[Instr] = Phi;
        ReMapper[Instr] = Phi;
        ReplaceInstWithInst(Tail, IIT, Phi);
    }
    for (llvm::Instruction* I : ToRemove)
        I->eraseFromParent();
    ++DuplicateBBCount;
}
llvm::PreservedAnalyses DuplicateB::run(llvm::Function& F, llvm::FunctionAnalysisManager& FAM) {
    if (!pRNG)
        pRNG = F.getParent()->createRNG("duplicate-bb");
    std::vector<std::tuple<llvm::BasicBlock*, llvm::Value*>> Targets = findBBsToDuplicate(F, FAM.getResult<RIV>(F));
    std::map<llvm::Value*, llvm::Value*> ReMapper;
    for (std::tuple<llvm::BasicBlock*, llvm::Value*>& BB_Ctx : Targets)
        cloneBB(*std::get<0>(BB_Ctx), std::get<1>(BB_Ctx), ReMapper);
    DuplicateBBCountStats = DuplicateBBCount;
    return (Targets.empty() ? llvm::PreservedAnalyses::all() : llvm::PreservedAnalyses::none());
}
llvm::PassPluginLibraryInfo getDuplicateBBPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "DuplicateB", LLVM_VERSION_STRING,
        [](llvm::PassBuilder& PB) {
            PB.registerPipelineParsingCallback(
                [](
                    llvm::StringRef Name, llvm::FunctionPassManager& FPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>
                ) {
                    if (Name == "duplicate-b") {
                        FPM.addPass(DuplicateB());
                        return true;
                    }
                    return false;
                }
            );
        }
    };
}
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getDuplicateBBPluginInfo();
}
