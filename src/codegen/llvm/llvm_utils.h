#pragma once
#include "llvm_context.h"
#include "llvm_value.h"
#include "parser/visitors/type_check_visitor/resolved_type.h"
#include <string>

namespace codegen {

/**
 * @namespace LLVMUtils
 * @brief Utility functions for LLVM code generation
 */
namespace LLVMUtils {

/**
 * @brief Creates a global string constant
 * @param context The LLVM context
 * @param str The string value
 * @param name Optional name for the global
 * @return LLVM value for the global string
 */
llvm::Value *createGlobalString(LLVMContext &context, const std::string &str,
                                const std::string &name = "");

/**
 * @brief Creates an integer constant
 * @param context The LLVM context
 * @param value The integer value
 * @param bits The bit width (default: 32)
 * @return LLVM value for the integer constant
 */
llvm::Value *createIntConstant(LLVMContext &context, int64_t value,
                               unsigned bits = 32);

/**
 * @brief Creates a floating-point constant
 * @param context The LLVM context
 * @param value The float value
 * @param isDouble Whether to create a double (64-bit) or float (32-bit)
 * @return LLVM value for the float constant
 */
llvm::Value *createFloatConstant(LLVMContext &context, double value,
                                 bool isDouble = false);

/**
 * @brief Creates a boolean constant
 * @param context The LLVM context
 * @param value The boolean value
 * @return LLVM value for the boolean constant
 */
llvm::Value *createBoolConstant(LLVMContext &context, bool value);

/**
 * @brief Creates a NULL pointer constant of the specified type
 * @param context The LLVM context
 * @param type The pointer type
 * @return LLVM value for the NULL pointer
 */
llvm::Value *createNullPointer(LLVMContext &context, llvm::PointerType *type);

/**
 * @brief Performs a numeric cast between primitive types
 * @param context The LLVM context
 * @param value The value to cast
 * @param toType The target type
 * @return The casted value
 */
LLVMValue castNumeric(LLVMContext &context, const LLVMValue &value,
                      std::shared_ptr<visitors::ResolvedType> toType);

/**
 * @brief Validates if a cast is permitted
 * @param fromType The source type
 * @param toType The target type
 * @return True if the cast is valid
 */
bool canCast(std::shared_ptr<visitors::ResolvedType> fromType,
             std::shared_ptr<visitors::ResolvedType> toType);

/**
 * @brief Emits a runtime error at the current insert point
 * @param context The LLVM context
 * @param message The error message
 */
void emitRuntimeError(LLVMContext &context, const std::string &message);

/**
 * @brief Gets the mangled name for a function
 * @param name The function name
 * @param paramTypes The parameter types
 * @return The mangled name
 */
std::string mangleFunctionName(
    const std::string &name,
    const std::vector<std::shared_ptr<visitors::ResolvedType>> &paramTypes);

} // namespace LLVMUtils

} // namespace codegen