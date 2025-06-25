#include "codegen/llvm/llvm_optimizer.h"
#include "llvm/Passes/PassBuilder.h"
#include <llvm/Pass.h>

namespace codegen {

LLVMOptimizer::LLVMOptimizer(LLVMContext &context)
    : context_(context), level_(OptimizationLevel::O0) {}

void LLVMOptimizer::setOptimizationLevel(OptimizationLevel level) {
  level_ = level;
}

void LLVMOptimizer::optimizeFunctions() {
  llvm::PassBuilder passBuilder;

  // Create and register analysis managers
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  // Register all the basic analyses with each manager
  passBuilder.registerModuleAnalyses(MAM);
  passBuilder.registerCGSCCAnalyses(CGAM);
  passBuilder.registerFunctionAnalyses(FAM);
  passBuilder.registerLoopAnalyses(LAM);
  passBuilder.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  // Create the pass manager
  llvm::FunctionPassManager FPM =
      passBuilder.buildFunctionSimplificationPipeline(
          llvmOptimizationLevel(),
          llvm::ThinOrFullLTOPhase::None // Debug logging
      );

  // Run on each function
  for (auto &F : context_.getModule()) {
    if (!F.isDeclaration()) {
      FPM.run(F, FAM);
    }
  }
}
// void LLVMOptimizer::optimizeModule() {
//     // Create a new pass manager
//     llvm::PassManager MPM;

//     // Add module-level passes
//     if (level_ >= OptimizationLevel::O2) {
//         MPM.add(llvm::createFunctionInliningPass());
//         // Add more passes as needed
//     }

//     // Run the passes
//     MPM.run(context_.getModule());
// }

void LLVMOptimizer::optimizeAll() {
  optimizeFunctions();
  //   optimizeModule();
}

llvm::OptimizationLevel LLVMOptimizer::llvmOptimizationLevel() const {
  switch (level_) {
  case OptimizationLevel::O0:
    return llvm::OptimizationLevel::O0;
  case OptimizationLevel::O1:
    return llvm::OptimizationLevel::O1;
  case OptimizationLevel::O2:
    return llvm::OptimizationLevel::O2;
  case OptimizationLevel::O3:
    return llvm::OptimizationLevel::O3;
  case OptimizationLevel::Os:
    return llvm::OptimizationLevel::Os;
  case OptimizationLevel::Oz:
    return llvm::OptimizationLevel::Oz;
  default:
    return llvm::OptimizationLevel::O0;
  }
}

} // namespace codegen