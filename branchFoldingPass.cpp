#define DEBUG_TYPE "BranchFoldingPass"
#include <map>
#include "constObjects.cpp"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"


//NOTE : this algorithm 
//opt -load $LLVMLIB/CSE231.so -ConstPass -BranchFoldingPass < $BENCHMARKS/gcd/gcd.bc > temp.instrumented.bc

using namespace llvm;
namespace {
	static IRBuilder<> builder(getGlobalContext());
	
  struct BranchFoldingPass : public ModulePass {
    static char ID; // Pass identification, replacement for typeid

    //iterate through all instructions, fill map.
    BranchFoldingPass() : ModulePass(ID) {
			//errs() << "Branch Folding Module Created" << "\n";
		}
    
    bool runOnModule(Module &M){
      
      //create the worklist object
      const_workListObj* wl = new const_workListObj(10010);
      wl->init(M);
      wl->run();

      //M.dump();
      //wl->printTop();
      //wl->printBottom();

      //For each BBNode
      for(map<int, const_BBNode*>::iterator it = wl->bbMap.begin(); it != wl->bbMap.end(); it++) {
        const_BBNode * BBN = it->second;
        const_Node * N = BBN->nodes.back();
        if(N->oI->getOpcode() == Instruction::Br) {
          //get the terminating instruction
          BranchInst * B = dyn_cast<BranchInst>(N->oI);
          //make sure the branch is conditional
          if(B->isConditional()) {
            //use the worklist results to replace input if available
            Value * temp = N->oI->getOperand(0);
            //check if the operand is linked to an instruction
            //NOTE: if it is already a constant like true or false, it will just skip this.
            Instruction * cond = dyn_cast<Instruction>(temp);
            if(cond && N->e->data->count(temp) == 1 ){
              const_Fact * c = N->e->data->find(cond)->second;
              if(c->possibleValues.size() == 1){
                set<Value *>::iterator itl = c->possibleValues.begin();
                Value * result = *itl;
                BasicBlock::iterator ii(cond);
                ReplaceInstWithValue(cond->getParent()->getInstList(), ii, result);
              }
            }
            //check for if it is true or false (0, or non-zero...)
            if(isa<ConstantInt>(B->getCondition())){
              ConstantInt * CI = dyn_cast<ConstantInt>(B->getCondition());
              //check false:
              if(CI->isZero()){
                //swap successors
                B->swapSuccessors();
              }
              //get the good black (always taken).
              BranchInst* New = BranchInst::Create(B->getSuccessor(0));
              //replace this branching instruction with an unconditional one
              ReplaceInstWithInst(B, New);
            }
          }
        }
      }
      M.dump();
			return false;
    }
  };
}

char BranchFoldingPass::ID = 0;
static RegisterPass<BranchFoldingPass> X("BranchFoldingPass", "Branch Folding Pass",false,false);
