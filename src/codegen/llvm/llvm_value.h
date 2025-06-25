#pragma once
#include "parser/visitors/type_check_visitor/resolved_type.h"
#include "llvm/IR/Value.h"
#include <llvm/IR/IRBuilder.h>
#include <memory>
#include <string>

namespace codegen {

/**
 * @class LLVMValue
 * @brief Represents a value in LLVM IR generation
 *
 * This class encapsulates an LLVM value along with its TS++ type information
 * and additional metadata needed during code generation.
 */
class LLVMValue {
public:
  /**
   * @brief Default constructor for an empty value
   */
  LLVMValue();

  /**
   * @brief Constructs a value with an LLVM value and type
   * @param value The LLVM value
   * @param type The TS++ type
   * @param isLValue Whether this is an lvalue (addressable)
   */
  LLVMValue(llvm::Value *value, std::shared_ptr<visitors::ResolvedType> type,
            bool isLValue = false);

  /**
   * @brief Gets the LLVM value
   * @return The LLVM value
   */
  llvm::Value *getValue() const { return value_; }

  /**
   * @brief Gets the TS++ type
   * @return The TS++ type
   */
  std::shared_ptr<visitors::ResolvedType> getType() const { return type_; }

  /**
   * @brief Checks if this is an lvalue
   * @return True if this is an lvalue
   */
  bool isLValue() const { return isLValue_; }

  /**
   * @brief Checks if this value is valid
   * @return True if this value is valid
   */
  bool isValid() const { return value_ != nullptr && type_ != nullptr; }

  /**
   * @brief Loads the value if it's an lvalue
   * @param builder The LLVM builder to use for loading
   * @return A new LLVMValue containing the loaded value
   */
  LLVMValue loadIfLValue(llvm::IRBuilder<> &builder) const;

  /**
   * @brief Gets a string representation of this value (for debugging)
   * @return String representation
   */
  std::string toString() const;

private:
  llvm::Value *value_;                           // The LLVM value
  std::shared_ptr<visitors::ResolvedType> type_; // The TS++ type
  bool isLValue_;                                // Whether this is an lvalue
};

} // namespace codegen