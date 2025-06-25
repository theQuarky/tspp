#pragma once
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <memory>
#include <string>

namespace codegen {

/**
 * @class LLVMContext
 * @brief Manages the LLVM context and module for code generation
 *
 * This class wraps the LLVM context, module, and builder, providing a central
 * repository for all LLVM-related state during code generation. It serves as
 * the main entry point for the code generation phase.
 */
class LLVMContext {
public:
  /**
   * @brief Constructs an LLVM context with the given module name
   * @param moduleName Name of the LLVM module
   */
  explicit LLVMContext(const std::string &moduleName = "tspp_module");

  /**
   * @brief Destructor to ensure proper cleanup of LLVM resources
   */
  ~LLVMContext();

  /**
   * @brief Gets the underlying LLVM context
   * @return Reference to the LLVM context
   */
  llvm::LLVMContext &getContext() { return *context_; }

  /**
   * @brief Gets the current module
   * @return Reference to the current LLVM module
   */
  llvm::Module &getModule() { return *module_; }

  /**
   * @brief Gets the IR builder for creating instructions
   * @return Reference to the LLVM IR builder
   */
  llvm::IRBuilder<> &getBuilder() { return *builder_; }

  /**
   * @brief Creates a new module, replacing the current one
   * @param moduleName Name of the new module
   */
  void createNewModule(const std::string &moduleName);

  /**
   * @brief Dumps the current module IR to the console (for debugging)
   */
  void dumpModule() const;

  /**
   * @brief Writes the module IR to a file
   * @param filename Path to the output file
   * @return True if the operation was successful
   */
  bool writeModuleToFile(const std::string &filename) const;

  /**
   * @brief Gets a string representation of the module IR
   * @return String containing the module IR
   */
  std::string getModuleIR() const;

private:
  std::unique_ptr<llvm::LLVMContext> context_; // LLVM context
  std::unique_ptr<llvm::Module> module_;       // Current module
  std::unique_ptr<llvm::IRBuilder<>> builder_; // IR instruction builder
};

} // namespace codegen