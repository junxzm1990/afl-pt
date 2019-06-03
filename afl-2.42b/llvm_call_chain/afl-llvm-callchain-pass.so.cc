/*
   american fuzzy lop - LLVM-call-chain-mode instrumentation pass
   ---------------------------------------------------

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.

 */

#define AFL_LLVM_CALL_CHAIN_PASS

#include "../config.h"
#include "../debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <functional>

#include "llvm/IR/Instructions.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
typedef std::map<std::string, int> FunctionMap;

static cl::opt<std::string> FunctionMapFilename("ofname", cl::Hidden, cl::desc("Specify \
output function map filename"), cl::init("fnmap.txt"));

namespace {

  class AFLCallChain: public ModulePass {

    public:

      static char ID;
      static std::ofstream ofmap;
      AFLCallChain() : ModulePass(ID) { }

      bool runOnModule(Module &M) override;
      bool instrumentRet(Instruction *I);
      bool instrumentCall(Instruction *I, std::string name);

      // StringRef getPassName() const override {
      //  return "American Fuzzy Lop Instrumentation";
      // }

  };

}


char AFLCallChain::ID = 0;
std::ofstream AFLCallChain::ofmap;


bool AFLCallChain::runOnModule(Module &M) {
    ofmap.open(FunctionMapFilename, std::ios_base::app);
    ofmap <<"Module Name" << ":" << M.getModuleIdentifier() << "\n";

#if 0
  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {

    SAYF(cCYA "afl-llvm-pass " cBRI VERSION cRST " by <lszekeres@google.com>\n");

  } else be_quiet = 1;

  /* Decide instrumentation ratio */

  char* inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");

  }

  /* Get globals for the SHM region and the previous location. Note that
     __afl_prev_loc is thread-local. */

  GlobalVariable *AFLMapPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");

  GlobalVariable *AFLPrevLoc = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc",
      0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  /* Instrument all the things! */

  int inst_blocks = 0;
#endif

  for (Function &F : M) {
    /* instrument the first BB of each function to log function call */
    if (F.empty()) {
        //errs() << "F.empty() is true\n";
        continue;
    }

    std::string name = F.getName().str();
    BasicBlock &entryBB = F.getEntryBlock();

    BasicBlock::iterator cIP = entryBB.getFirstInsertionPt();
    instrumentCall(&(*cIP), name);

    for (BasicBlock &BB : F) {

      if (ReturnInst *RI = dyn_cast<ReturnInst>(BB.getTerminator())) {  
        instrumentRet(RI);
      }
    }
  }

  ofmap.close();

  return true;
}

bool AFLCallChain::instrumentCall(Instruction *I, std::string name) {
    int fid;
    std:: string moduleName;
    std::hash<std::string> str_hash;
    IRBuilder<> IRB(I);
    Module *M = IRB.GetInsertBlock()->getModule();
    Type *VoidTy = IRB.getVoidTy();
    Type *I32Ty = IRB.getInt32Ty();
    ConstantInt *constFid;

    // check whether we have visit this function before
    moduleName = M->getModuleIdentifier();
    fid = (int)(str_hash(moduleName + name) & 0x7fffffff);
    ofmap << name << ":" << fid << "\n";

    //errs() << "szc: fid :" << fid << " name:"<<name<<"\n";

    Constant *ConstLogCall = M->getOrInsertFunction("__afl_log_call", VoidTy,
                                                                     I32Ty
                                                                     );
    Function *FuncLogCall = cast<Function>(ConstLogCall);
    constFid = ConstantInt::get((IntegerType*)I32Ty, fid);
    IRB.CreateCall(FuncLogCall, {constFid});

    return true;
}

bool AFLCallChain::instrumentRet(Instruction *I) {
    IRBuilder<> B(I);
    Module *M = B.GetInsertBlock()->getModule();
    Type *VoidTy = B.getVoidTy();

    Constant *ConstLogRet = M->getOrInsertFunction("__afl_log_ret", VoidTy
                                                               );
    Function *FuncLogRet = cast<Function>(ConstLogRet);
    B.CreateCall(FuncLogRet,None); 

    return true;
}

#if 0
      BasicBlock::iterator IP = BB.getFirstInsertionPt();
      IRBuilder<> IRB(&(*IP));

      if (AFL_R(100) >= inst_ratio) continue;

      /* Make up cur_loc */

      unsigned int cur_loc = AFL_R(MAP_SIZE);

      ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

      /* Load prev_loc */

      LoadInst *PrevLoc = IRB.CreateLoad(AFLPrevLoc);
      PrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

      /* Load SHM pointer */

      LoadInst *MapPtr = IRB.CreateLoad(AFLMapPtr);
      MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *MapPtrIdx =
          IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));

      /* Update bitmap */

      LoadInst *Counter = IRB.CreateLoad(MapPtrIdx);
      Counter->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
      IRB.CreateStore(Incr, MapPtrIdx)
          ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      /* Set prev_loc to cur_loc >> 1 */

      StoreInst *Store =
          IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
      Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      inst_blocks++;

    }

  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks) WARNF("No instrumentation targets found.");
    else OKF("Instrumented %u locations (%s mode, ratio %u%%).",
             inst_blocks, getenv("AFL_HARDEN") ? "hardened" :
             ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN")) ?
              "ASAN/MSAN" : "non-hardened"), inst_ratio);

  }

  return true;

}

#endif

static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCallChain());

}


static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_OptimizerLast, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);
