#pragma once
#include "llvm_context.h"
#include "llvm_value.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace codegen {

/**
 * @class LLVMFunction
 * @brief Manages LLVM function generation
 *
 * This class encapsulates function generation, including parameter handling,
 * variable scoping, and basic block management.
 */
class LLVMFunction {
public:
  /**
   * @brief Constructs a function generator
   * @param context The LLVM context
   * @param func The LLVM function
   * @param returnType The function return type
   */
  LLVMFunction(LLVMContext& context, 
               llvm::Function* func,
               std::shared_ptr<visitors::ResolvedType> returnType);

  /**
   * @brief Gets the LLVM function
   * @return The LLVM function
   */
  llvm::Function* getFunction() const { return function_; }
  
  /**
   * @brief Gets the function return type
   * @return The function return type
   */
  std::shared_ptr<visitors::ResolvedType> getReturnType() const { return returnType_; }

  /**
   * @brief Creates and enters a new scope
   */
  void enterScope();
  
  /**
   * @brief Exits the current scope
   */
  void exitScope();

  /**
   * @brief Declares a variable in the current scope
   * @param name The variable name
   * @param value The variable value
   */
  void declareVariable(const std::string& name, const LLVMValue& value);
  
  /**
   * @brief Looks up a variable in the current and parent scopes
   * @param name The variable name
   * @return The variable value, or an invalid value if not found
   */
  LLVMValue getVariable(const std::string& name) const;

  /**
   * @brief Creates a new basic block
   * @param name The block name
   * @return The created basic block
   */
  llvm::BasicBlock* createBasicBlock(const std::string& name);
  
  /**
   * @brief Sets the current insert point to the given block
   * @param block The block to set as insert point
   */
  void setInsertPoint(llvm::BasicBlock* block);

  /**
   * @brief Creates a function parameter map
   * @param paramNames Parameter names
   */
  void mapParameters(const std::vector<std::string>& paramNames);

private:
  LLVMContext& context_;  // The LLVM context
  llvm::Function* function_;  // The LLVM function
  std::shared_ptr<visitors::ResolvedType> returnType_;  // Function return type
  
  // Variable scopes, with innermost scope at the back
  struct Scope {
    std::unordered_map<std::string, LLVMValue> variables;
  };
  std::vector<Scope> scopes_;
};

} // namespace codegen