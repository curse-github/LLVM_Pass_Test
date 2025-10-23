#define _BUILD_DUP_B
#include "DuplicateB.h"

#include <iostream>

llvm::PreservedAnalyses DuplicateB::run(llvm::Function& F, llvm::FunctionAnalysisManager& FAM) {
    // create a randmon number generator
    if (!RngGen)
        RngGen = F.getParent()->createRNG("duplicate-b");
    // call the findDuplicatableBs function to find blocks to then duplicate
    const RIV::Result& RIVResult = FAM.getResult<RIV>(F);
    std::vector<std::pair<llvm::BasicBlock*, llvm::Value*>> B_Tgt_s = findDuplicatableBs(F, RIVResult);
    std::unordered_map<llvm::Value*, llvm::Value*> ValRe_mapper;
    for (std::pair<llvm::BasicBlock*, llvm::Value*>& B_Tgt : B_Tgt_s)
        duplicateB(B_Tgt.first, B_Tgt.second, ValRe_mapper);
    return (B_Tgt_s.empty() ? llvm::PreservedAnalyses::all() : llvm::PreservedAnalyses::none());
}


std::vector<std::pair<llvm::BasicBlock*, llvm::Value*>> DuplicateB::findDuplicatableBs(llvm::Function& F, const RIV::Result& RIVResult) {
    std::vector<std::pair<llvm::BasicBlock*, llvm::Value*>> BlocksToDuplicate;
    // loop through each block in that function
    for (llvm::BasicBlock& B : F) {
        // skip excpetion
        if (B.isLandingPad())
            continue;
        // fetch the reachable integer values (RIV)
        const llvm::SmallPtrSet<llvm::Value*, 8U>& ReachableValues = RIVResult.lookup(&B);
        // if there isnt any, skip that block
        size_t ReachableValuesCount = ReachableValues.size();
        if (0 == ReachableValuesCount)
            continue;
        // get a random one of the reachable values
        llvm::SmallPtrSetIterator<llvm::Value*> Iter = ReachableValues.begin();
        std::uniform_int_distribution<> Dist(0, ReachableValuesCount - 1);
        std::advance(Iter, Dist(*RngGen));
        if (llvm::dyn_cast<llvm::GlobalValue>(*Iter))
            continue;
        // push a pair of the 
        BlocksToDuplicate.emplace_back(&B, *Iter);
    }
    return BlocksToDuplicate;
}
void DuplicateB::duplicateB(llvm::BasicBlock* B, llvm::Value* TargetVal, std::unordered_map<llvm::Value*, llvm::Value*>& ValRe_mapper) {
    llvm::Instruction* Btop = B->getFirstNonPHI();
    // save original phi instruction and block names to fix them later
    std::string BName = B->getName().str();
    std::vector<std::string> phiNames{};
    for(llvm::Instruction* Inst = &*B->begin(); Inst != Btop; Inst = Inst->getNextNode())
        phiNames.push_back(Inst->getName().str());
    // create if else block at the location of the target value, or what the target value has now changed to
    llvm::Value* Val = (ValRe_mapper.count(TargetVal) > 0) ? ValRe_mapper[TargetVal] : TargetVal;
    llvm::CmpInst* Cond = llvm::ICmpInst::Create(llvm::Instruction::OtherOps::ICmp, llvm::ICmpInst::Predicate::ICMP_EQ, Val, llvm::ConstantInt::get(Val->getType(), 0), "cond");
    Cond->insertBefore(Btop);
    llvm::Instruction* ifTerm = nullptr;
    llvm::Instruction* elseTerm = nullptr;
    SplitBlockAndInsertIfThenElse(Cond, Btop, &ifTerm, &elseTerm);
    // get the BasicBlocks
    llvm::BasicBlock* headBranch = ifTerm->getParent()->getSinglePredecessor();
    llvm::BasicBlock* ifBranch = ifTerm->getParent();
    llvm::BasicBlock* elseBranch = elseTerm->getParent();
    llvm::BasicBlock* tailBranch = elseTerm->getParent()->getSingleSuccessor();
    // fix names of phi instructions and blocks
    Btop = headBranch->getFirstNonPHI();
    unsigned int i = 0;
    for (llvm::Instruction* Inst = &*headBranch->begin(); Inst != Btop; Inst = Inst->getNextNode()) {
        Inst->setName(phiNames[i]); i++;
    }
    headBranch->setName(BName);
    ifBranch->setName(BName + ".if");
    elseBranch->setName(BName + ".else");
    tailBranch->setName(BName + ".tail");
    // loop through instructions in tail which just contains the original data of the block
    // duplicate each instruction into the two branches except the terminator instruction
    llvm::ValueToValueMapTy fullValMap{}, ifValMap{}, ElseValMap{};
    llvm::SmallVector<llvm::Instruction*, 8> ToRemove;
    llvm::BasicBlock::iterator IE = tailBranch->end();
    for (llvm::BasicBlock::iterator IIT = tailBranch->begin(); IIT != IE; ++IIT) {
        llvm::Instruction* Inst = &*IIT;
        // if it is the terminator, leave it in the tail branch
        if (Inst->isTerminator()) {
            llvm::RemapInstruction(Inst, fullValMap, llvm::RF_IgnoreMissingLocals);
            break;
        }
        // clone the instruction, re-map it, and insert it into the if block
        llvm::Instruction* ifClone = Inst->clone();
        llvm::RemapInstruction(ifClone, ifValMap, llvm::RF_IgnoreMissingLocals);
        ifClone->insertBefore(ifTerm);
        ifValMap[Inst] = ifClone;
        // clone the instruction, re-map it, and insert it into the else block
        llvm::Instruction* elseClone = Inst->clone();
        llvm::RemapInstruction(elseClone, ElseValMap, llvm::RF_IgnoreMissingLocals);
        elseClone->insertBefore(elseTerm);
        ElseValMap[Inst] = elseClone;
        // if the instruction has no uses, it doesnt need a phi instruction
        if (Inst->use_begin() == Inst->use_end()) {
            ToRemove.push_back(Inst);
            continue;
        }
        std::string instName = Inst->getName().str();
        // replace original statement in tail with phi of two cloned values
        llvm::PHINode* Phi = llvm::PHINode::Create(Inst->getType(), 2);
        Phi->addIncoming(ifClone, ifBranch);
        Phi->addIncoming(elseClone, elseBranch);
        llvm::ReplaceInstWithInst(tailBranch, IIT, Phi);
        // fix names
        Inst->setName(instName + ".old");
        ifClone->setName(instName + ".if");
        elseClone->setName(instName + ".else");
        Phi->setName(instName);
        // save phi instruction to replace the original in other places
        ValRe_mapper[Inst] = fullValMap[Inst] = Phi;
    }
    for (llvm::Instruction* I : ToRemove)
        I->eraseFromParent();
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
#pragma comment(linker, "/EXPORT:llvmGetPassPluginInfo")
extern "C"
llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getDuplicateBPluginInfo();
}
