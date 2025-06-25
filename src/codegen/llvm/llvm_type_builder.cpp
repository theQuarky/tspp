#include "codegen/llvm/llvm_type_builder.h"
#include "parser/visitors/type_check_visitor/resolved_type.h"

namespace codegen {

LLVMTypeBuilder::LLVMTypeBuilder(LLVMContext &context) : context_(context) {
  // Register basic types
  typeCache_["void"] = llvm::Type::getVoidTy(context_.getContext());
  typeCache_["int"] = llvm::Type::getInt32Ty(context_.getContext());
  typeCache_["float"] = llvm::Type::getFloatTy(context_.getContext());
  typeCache_["bool"] = llvm::Type::getInt1Ty(context_.getContext());
  typeCache_["string"] = llvm::PointerType::getUnqual(
      llvm::Type::getInt8Ty(context_.getContext()));
}

llvm::Type *LLVMTypeBuilder::convertType(
    const std::shared_ptr<visitors::ResolvedType> &type) {
  if (!type) {
    return llvm::Type::getVoidTy(context_.getContext());
  }

  switch (type->getKind()) {
  case visitors::ResolvedType::TypeKind::Void:
    return llvm::Type::getVoidTy(context_.getContext());

  case visitors::ResolvedType::TypeKind::Int:
    return llvm::Type::getInt32Ty(context_.getContext());

  case visitors::ResolvedType::TypeKind::Float:
    return llvm::Type::getFloatTy(context_.getContext());

  case visitors::ResolvedType::TypeKind::Bool:
    return llvm::Type::getInt1Ty(context_.getContext());

  case visitors::ResolvedType::TypeKind::String:
    return llvm::PointerType::getUnqual(
        llvm::Type::getInt8Ty(context_.getContext()));

  case visitors::ResolvedType::TypeKind::Named: {
    auto typeName = type->getName();
    auto it = typeCache_.find(typeName);
    if (it != typeCache_.end()) {
      return it->second;
    }
    // Create a forward declaration for the type if it doesn't exist
    auto structType = llvm::StructType::create(context_.getContext(), typeName);
    typeCache_[typeName] = structType;
    return structType;
  }

  case visitors::ResolvedType::TypeKind::Array: {
    auto elemType = convertType(type->getElementType());
    // Use a pointer for now, as LLVM arrays are fixed size
    return llvm::PointerType::getUnqual(elemType);
  }

  case visitors::ResolvedType::TypeKind::Pointer: {
    auto pointeeType = convertType(type->getPointeeType());
    return llvm::PointerType::getUnqual(pointeeType);
  }

  case visitors::ResolvedType::TypeKind::Function: {
    auto returnType = convertType(type->getReturnType());
    std::vector<llvm::Type *> paramTypes;
    for (const auto &paramType : type->getParameterTypes()) {
      paramTypes.push_back(convertType(paramType));
    }
    return llvm::PointerType::getUnqual(
        llvm::FunctionType::get(returnType, paramTypes, false));
  }

  default:
    // For any other type, return a generic pointer as a placeholder
    return llvm::PointerType::getUnqual(
        llvm::Type::getInt8Ty(context_.getContext()));
  }
}

llvm::Type *LLVMTypeBuilder::getTypeByName(const std::string &typeName) {
  auto it = typeCache_.find(typeName);
  if (it != typeCache_.end()) {
    return it->second;
  }
  return nullptr;
}

llvm::StructType *LLVMTypeBuilder::createStructType(
    const std::string &typeName,
    const std::vector<std::pair<std::string, llvm::Type *>> &fields) {

  // Collect field types
  std::vector<llvm::Type *> fieldTypes;
  std::unordered_map<std::string, size_t> fieldIndices;

  for (size_t i = 0; i < fields.size(); ++i) {
    fieldTypes.push_back(fields[i].second);
    fieldIndices[fields[i].first] = i;
  }

  // Check if the type already exists as a forward declaration
  llvm::StructType *structType = nullptr;
  auto it = typeCache_.find(typeName);
  if (it != typeCache_.end() && llvm::isa<llvm::StructType>(it->second)) {
    structType = llvm::cast<llvm::StructType>(it->second);
    if (structType->isOpaque()) {
      structType->setBody(fieldTypes);
    }
  } else {
    // Create a new struct type
    structType =
        llvm::StructType::create(context_.getContext(), fieldTypes, typeName);
    typeCache_[typeName] = structType;
  }

  // Store the field indices
  structInfo_[typeName] = {structType, std::move(fieldIndices)};

  return structType;
}

int LLVMTypeBuilder::getFieldIndex(const std::string &structType,
                                   const std::string &fieldName) {
  auto structIt = structInfo_.find(structType);
  if (structIt == structInfo_.end()) {
    return -1;
  }

  auto fieldIt = structIt->second.fieldIndices.find(fieldName);
  if (fieldIt == structIt->second.fieldIndices.end()) {
    return -1;
  }

  return static_cast<int>(fieldIt->second);
}

} // namespace codegen