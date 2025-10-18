#include "DuplicateB.h"

std::vector<std::pair<llvm::BasicBlock*, llvm::Value*>> DuplicateB::findDuplicatableBs(llvm::Function& F, const RIV::Result& RIVResult) {
    std::vector<std::pair<llvm::BasicBlock*, llvm::Value*>> BlocksToDuplicate;
    // loop through each block in that function
    for (llvm::BasicBlock& B : F) {
        // skip excpetion
        if (B.isLandingPad())
            continue;
        // fetch the reachable integer values (RIV)
        const llvm::SmallPtrSet<llvm::Value *, 8U>& ReachableValues = RIVResult.lookup(&B);
        // if there isnt any, skip that block
        size_t ReachableValuesCount = ReachableValues.size();
        if (0 == ReachableValuesCount)
            continue;
        // get a random one of the reachable values
        llvm::SmallPtrSetIterator<llvm::Value *> Iter = ReachableValues.begin();
        std::uniform_int_distribution<> Dist(0, ReachableValuesCount - 1);
        std::advance(Iter, Dist(*RngGen));
        if (llvm::dyn_cast<llvm::GlobalValue>(*Iter))
            continue;
        // push a pair of the 
        BlocksToDuplicate.emplace_back(&B, *Iter);
    }
    return BlocksToDuplicate;
}
void DuplicateB::duplicateB(llvm::BasicBlock* B, llvm::Value* TargetVal, std::map<llvm::Value*, llvm::Value*>& ValRe_mapper) {
    llvm::Instruction* Btop = &*(B->getFirstNonPHIIt());
    llvm::Twine BName = ""+B->getName();
    llvm::IRBuilder<> Builder(&*Btop);
    // create if else block at the location of the target value, or what the target value has now changed to
    llvm::Value* Cond = Builder.CreateIsNull((ValRe_mapper.count(TargetVal) > 0) ? ValRe_mapper[TargetVal] : TargetVal);
    llvm::Instruction* ifTerm = nullptr;
    llvm::Instruction* elseTerm = nullptr;
    SplitBlockAndInsertIfThenElse(Cond, &*Btop, &ifTerm, &elseTerm);
    llvm::BasicBlock* headBranch = ifTerm->getParent()->getSinglePredecessor();
    llvm::BasicBlock* ifBranch = ifTerm->getParent();
    llvm::BasicBlock* elseBranch = elseTerm->getParent();
    llvm::BasicBlock* tailBranch = elseTerm->getSuccessor(0);
    headBranch->setName(BName);// getSinglePredecessor will return nullptr if there is more than one
    ifBranch->setName(BName + ".if");
    elseBranch->setName(BName + ".else");
    tailBranch->setName(BName + ".tail");
    // loop through instructions in tail which just contains the original data of the block
    // duplicate each instruction into the two branches except the terminator instruction
    llvm::ValueToValueMapTy fullValMap{}, ifValMap{}, ElseValMap{};
    llvm::SmallVector<llvm::Instruction*, 8> ToRemove;
    llvm::BasicBlock::iterator IE = tailBranch->end();
    for (llvm::BasicBlock::iterator IIT = tailBranch->begin(); IIT != IE; ++IIT) {
        // get instruction
        llvm::Instruction* Instr = &*IIT;
        assert(!llvm::isa<llvm::PHINode>(Instr) && "Phi nodes have already been filtered out");
        // if it is the Term, leave it in the tail branch
        if (Instr->isTerminator()) {
            llvm::RemapInstruction(Instr, fullValMap, llvm::RF_IgnoreMissingLocals);
            break;
        }
        // clone and re-map using already cloned values
        llvm::Instruction* ifClone = Instr->clone();
        llvm::RemapInstruction(ifClone, ifValMap, llvm::RF_IgnoreMissingLocals);
        // add to end of if block and insert in map
        ifClone->insertBefore(ifTerm);
        ifValMap[Instr] = ifClone;

        // clone and re-map using already cloned values
        llvm::Instruction* elseClone = Instr->clone();
        llvm::RemapInstruction(elseClone, ElseValMap, llvm::RF_IgnoreMissingLocals);
        // add to end of else block and insert in map
        elseClone->insertBefore(elseTerm);
        ElseValMap[Instr] = elseClone;

        // if the instruction returns void just duplicate it, dont need phi instruction
        if (Instr->getType()->isVoidTy()) {
            ToRemove.push_back(Instr);
            continue;
        }
        // set clone's names
        llvm::StringRef instrName = Instr->getName();
        ifClone->setName(instrName + ".if");
        elseClone->setName(instrName + ".else");
        // eplace original statement in tail with phi of two cloned values
        llvm::PHINode* Phi = llvm::PHINode::Create(Instr->getType(), 2);
        Phi->addIncoming(ifClone, ifBranch);
        Phi->addIncoming(elseClone, elseBranch);
        ReplaceInstWithInst(tailBranch, IIT, Phi);
        // save phi instruction to replace the original in other places
        ValRe_mapper[Instr] = fullValMap[Instr] = Phi;
    }
    for (llvm::Instruction* I : ToRemove)
        I->eraseFromParent();
}
llvm::PreservedAnalyses DuplicateB::run(llvm::Function& F, llvm::FunctionAnalysisManager& FAM) {
    // create a randmon number generator
    if (!RngGen)
        RngGen = F.getParent()->createRNG("duplicate-b");
    // call the findDuplicatableBs function to find blocks to then duplicate
    std::vector<std::pair<llvm::BasicBlock*, llvm::Value*>> B_Tgt_s = findDuplicatableBs(F, FAM.getResult<RIV>(F));
    std::map<llvm::Value*, llvm::Value*> ValRe_mapper;
    for (std::pair<llvm::BasicBlock*, llvm::Value*>& B_Tgt : B_Tgt_s)
        duplicateB(B_Tgt.first, B_Tgt.second, ValRe_mapper);
    return (B_Tgt_s.empty() ? llvm::PreservedAnalyses::all() : llvm::PreservedAnalyses::none());
}
llvm::PassPluginLibraryInfo getDuplicateBPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "DuplicateB", LLVM_VERSION_STRING,
        [](llvm::PassBuilder& passBuilder) {
            passBuilder.registerPipelineParsingCallback(
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
    return getDuplicateBPluginInfo();
}
