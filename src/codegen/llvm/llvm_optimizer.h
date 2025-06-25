#pragma once
#include "codegen/codegen_options.h" // Include for OptimizationLevel
#include "llvm_context.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include <memory>
#include <string>

namespace codegen {

/**
 * @class LLVMOptimizer
 * @brief Manages LLVM optimizations for generated code
 *
 * This class sets up and runs LLVM optimization passes on the IR.
 */
class LLVMOptimizer {
public:
  /**
   * @brief Constructs an optimizer for the given context
   * @param context The LLVM context
   */
  explicit LLVMOptimizer(LLVMContext &context);

  /**
   * @brief Sets the optimization level
   * @param level The optimization level
   */
  void setOptimizationLevel(OptimizationLevel level);

  /**
   * @brief Gets the current optimization level
   * @return The current optimization level
   */
  OptimizationLevel getOptimizationLevel() const { return level_; }

  /**
   * @brief Runs function-level optimizations on the module
   */
  void optimizeFunctions();

  /**
   * @brief Runs module-level optimizations on the module
   */
  void optimizeModule();

  /**
   * @brief Runs all optimizations on the module
   */
  void optimizeAll();

private:
  /**
   * @brief Converts our optimization level to LLVM's optimization level
   * @return The LLVM optimization level
   */
  llvm::OptimizationLevel llvmOptimizationLevel() const;

  LLVMContext &context_;    // The LLVM context
  OptimizationLevel level_; // Current optimization level
};

} // namespace codegen