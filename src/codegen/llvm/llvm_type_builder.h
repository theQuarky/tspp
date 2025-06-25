#pragma once
#include "llvm_context.h"
#include "parser/visitors/type_check_visitor/resolved_type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include <memory>
#include <unordered_map>

namespace codegen {

/**
 * @class LLVMTypeBuilder
 * @brief Translates TS++ types to LLVM IR types
 *
 * This class handles the conversion from the compiler's internal type
 * representation (ResolvedType) to the corresponding LLVM IR types.
 */
class LLVMTypeBuilder {
public:
  /**
   * @brief Constructs a type builder with the given LLVM context
   * @param context Reference to the LLVM context
   */
  explicit LLVMTypeBuilder(LLVMContext &context);

  /**
   * @brief Converts a TS++ type to an LLVM type
   * @param type The TS++ type to convert
   * @return The corresponding LLVM type
   */
  llvm::Type *convertType(const std::shared_ptr<visitors::ResolvedType> &type);

  /**
   * @brief Gets the LLVM type for a given TS++ type name
   * @param typeName Name of the TS++ type
   * @return The corresponding LLVM type, or nullptr if not found
   */
  llvm::Type *getTypeByName(const std::string &typeName);

  /**
   * @brief Registers a class/struct type with its fields
   * @param typeName Name of the class/struct
   * @param fields Field names and types
   * @return The created LLVM struct type
   */
  llvm::StructType *createStructType(
      const std::string &typeName,
      const std::vector<std::pair<std::string, llvm::Type *>> &fields);

  /**
   * @brief Gets the field index in a struct type
   * @param structType Name of the struct type
   * @param fieldName Name of the field
   * @return Index of the field or -1 if not found
   */
  int getFieldIndex(const std::string &structType,
                    const std::string &fieldName);

private:
  // Convert primitive types
  llvm::Type *convertPrimitiveType(const visitors::ResolvedType &type);

  // Convert array types
  llvm::Type *convertArrayType(const visitors::ResolvedType &type);

  // Convert pointer types
  llvm::Type *convertPointerType(const visitors::ResolvedType &type);

  // Convert function types
  llvm::Type *convertFunctionType(const visitors::ResolvedType &type);

  // Convert struct/class types
  llvm::Type *convertStructType(const visitors::ResolvedType &type);

  // Convert smart pointer types
  llvm::Type *convertSmartPointerType(const visitors::ResolvedType &type);

  LLVMContext &context_; // Reference to the LLVM context

  // Maps type names to their LLVM representation
  std::unordered_map<std::string, llvm::Type *> typeCache_;

  // Maps struct types to their field information
  struct StructInfo {
    llvm::StructType *type;
    std::unordered_map<std::string, size_t> fieldIndices;
  };
  std::unordered_map<std::string, StructInfo> structInfo_;
};

} // namespace codegen