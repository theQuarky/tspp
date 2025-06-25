#include "codegen/llvm/llvm_utils.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include <sstream>
#include <string>

namespace codegen {
namespace LLVMUtils {

llvm::Value *createGlobalString(LLVMContext &context, const std::string &str,
                                const std::string &name) {
  llvm::IRBuilder<> &builder = context.getBuilder();
  llvm::Module &module = context.getModule();

  // Create a global string constant
  llvm::Constant *strConstant = llvm::ConstantDataArray::getString(
      context.getContext(), str, true // Include null terminator
  );

  // Create a unique name if none is provided
  std::string varName = name;
  if (varName.empty()) {
    static int counter = 0;
    std::stringstream ss;
    ss << ".str." << counter++;
    varName = ss.str();
  }

  // Create a global variable for the string
  llvm::GlobalVariable *globalStr = new llvm::GlobalVariable(
      module, strConstant->getType(),
      true, // isConstant
      llvm::GlobalValue::PrivateLinkage, strConstant, varName);

  // Get a pointer to the first character
  llvm::Value *zero =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context.getContext()), 0);

  llvm::Value *indices[] = {zero, zero};
  return builder.CreateInBoundsGEP(globalStr->getValueType(), globalStr,
                                   indices, "str");
}

llvm::Value *createIntConstant(LLVMContext &context, int64_t value,
                               unsigned bits) {
  return llvm::ConstantInt::get(
      llvm::Type::getIntNTy(context.getContext(), bits), value,
      true // Signed
  );
}

llvm::Value *createFloatConstant(LLVMContext &context, double value,
                                 bool isDouble) {
  if (isDouble) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context.getContext()),
                                 value);
  } else {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(context.getContext()),
                                 static_cast<float>(value));
  }
}

llvm::Value *createBoolConstant(LLVMContext &context, bool value) {
  return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context.getContext()),
                                value ? 1 : 0);
}

llvm::Value *createNullPointer(LLVMContext &context, llvm::PointerType *type) {
  return llvm::ConstantPointerNull::get(type);
}

LLVMValue castNumeric(LLVMContext &context, const LLVMValue &value,
                      std::shared_ptr<visitors::ResolvedType> toType) {
  if (!value.isValid() || !toType) {
    return LLVMValue();
  }

  llvm::IRBuilder<> &builder = context.getBuilder();
  auto valueType = value.getType();
  auto valueLLVM = value.getValue();

  // Handle loading if the value is an lvalue
  if (value.isLValue()) {
    LLVMValue loaded = value.loadIfLValue(builder);
    return castNumeric(context, loaded, toType);
  }

  // No need to cast if types are the same
  if (valueType->equals(*toType)) {
    return value;
  }

  // Get LLVM types
  llvm::Type *fromLLVMType = valueLLVM->getType();
  llvm::Type *toLLVMType = nullptr;

  switch (toType->getKind()) {
  case visitors::ResolvedType::TypeKind::Int:
    toLLVMType = llvm::Type::getInt32Ty(context.getContext());
    break;
  case visitors::ResolvedType::TypeKind::Float:
    toLLVMType = llvm::Type::getFloatTy(context.getContext());
    break;
  case visitors::ResolvedType::TypeKind::Bool:
    toLLVMType = llvm::Type::getInt1Ty(context.getContext());
    break;
  default:
    // If not a primitive type, return as is
    return value;
  }

  // Perform the cast
  llvm::Value *castedValue = nullptr;

  // Integer to integer
  if (fromLLVMType->isIntegerTy() && toLLVMType->isIntegerTy()) {
    unsigned fromBits = fromLLVMType->getIntegerBitWidth();
    unsigned toBits = toLLVMType->getIntegerBitWidth();

    if (fromBits < toBits) {
      // Sign extend
      castedValue = builder.CreateSExt(valueLLVM, toLLVMType, "cast");
    } else if (fromBits > toBits) {
      // Truncate
      castedValue = builder.CreateTrunc(valueLLVM, toLLVMType, "cast");
    } else {
      // Same size, no-op
      castedValue = valueLLVM;
    }
  }
  // Float to float
  else if (fromLLVMType->isFloatingPointTy() &&
           toLLVMType->isFloatingPointTy()) {
    if (fromLLVMType->getPrimitiveSizeInBits() <
        toLLVMType->getPrimitiveSizeInBits()) {
      // Extend precision
      castedValue = builder.CreateFPExt(valueLLVM, toLLVMType, "cast");
    } else if (fromLLVMType->getPrimitiveSizeInBits() >
               toLLVMType->getPrimitiveSizeInBits()) {
      // Truncate precision
      castedValue = builder.CreateFPTrunc(valueLLVM, toLLVMType, "cast");
    } else {
      // Same size, no-op
      castedValue = valueLLVM;
    }
  }
  // Integer to float
  else if (fromLLVMType->isIntegerTy() && toLLVMType->isFloatingPointTy()) {
    // Signed integer to float
    castedValue = builder.CreateSIToFP(valueLLVM, toLLVMType, "cast");
  }
  // Float to integer
  else if (fromLLVMType->isFloatingPointTy() && toLLVMType->isIntegerTy()) {
    // Float to signed integer
    castedValue = builder.CreateFPToSI(valueLLVM, toLLVMType, "cast");
  }
  // Pointer to integer
  else if (fromLLVMType->isPointerTy() && toLLVMType->isIntegerTy()) {
    castedValue = builder.CreatePtrToInt(valueLLVM, toLLVMType, "cast");
  }
  // Integer to pointer
  else if (fromLLVMType->isIntegerTy() && toLLVMType->isPointerTy()) {
    castedValue = builder.CreateIntToPtr(valueLLVM, toLLVMType, "cast");
  }
  // Any other case
  else {
    // Try a bitcast as a last resort
    castedValue = builder.CreateBitCast(valueLLVM, toLLVMType, "cast");
  }

  return LLVMValue(castedValue, toType);
}

bool canCast(std::shared_ptr<visitors::ResolvedType> fromType,
             std::shared_ptr<visitors::ResolvedType> toType) {
  if (!fromType || !toType) {
    return false;
  }

  // Same type, always castable
  if (fromType->equals(*toType)) {
    return true;
  }

  // Check if implicitly convertible
  if (fromType->isImplicitlyConvertibleTo(*toType)) {
    return true;
  }

  // Check if explicitly convertible
  return fromType->isExplicitlyConvertibleTo(*toType);
}

void emitRuntimeError(LLVMContext &context, const std::string &message) {
  llvm::IRBuilder<> &builder = context.getBuilder();
  llvm::Module &module = context.getModule();

  // Get the printf function
  llvm::Function *printfFunc = module.getFunction("printf");
  if (!printfFunc) {
    // Declare printf if not already declared
    llvm::FunctionType *printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context.getContext()),
        {llvm::PointerType::get(llvm::Type::getInt8Ty(context.getContext()),
                                0)},
        true // varargs
    );

    printfFunc = llvm::Function::Create(
        printfType, llvm::Function::ExternalLinkage, "printf", module);
  }

  // Create the error message string
  std::string errorMsg = "Runtime error: " + message + "\n";
  llvm::Value *msgGlobal = createGlobalString(context, errorMsg);

  // Call printf with the error message
  builder.CreateCall(printfFunc, {msgGlobal});

  // Get the exit function
  llvm::Function *exitFunc = module.getFunction("exit");
  if (!exitFunc) {
    // Declare exit if not already declared
    llvm::FunctionType *exitType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context.getContext()),
        {llvm::Type::getInt32Ty(context.getContext())}, false);

    exitFunc = llvm::Function::Create(exitType, llvm::Function::ExternalLinkage,
                                      "exit", module);
  }

  // Call exit with status code 1
  builder.CreateCall(exitFunc,
                     {llvm::ConstantInt::get(
                         llvm::Type::getInt32Ty(context.getContext()), 1)});

  // Add unreachable instruction
  builder.CreateUnreachable();
}

std::string mangleFunctionName(
    const std::string &name,
    const std::vector<std::shared_ptr<visitors::ResolvedType>> &paramTypes) {

  std::stringstream mangled;

  // Start with the base name
  mangled << "_Z" << name.length() << name;

  // Append parameter types
  for (const auto &paramType : paramTypes) {
    if (!paramType) {
      continue;
    }

    // Simple parameter mangling based on type kind
    switch (paramType->getKind()) {
    case visitors::ResolvedType::TypeKind::Void:
      mangled << "v";
      break;
    case visitors::ResolvedType::TypeKind::Int:
      mangled << "i";
      break;
    case visitors::ResolvedType::TypeKind::Float:
      mangled << "f";
      break;
    case visitors::ResolvedType::TypeKind::Bool:
      mangled << "b";
      break;
    case visitors::ResolvedType::TypeKind::String:
      mangled << "PKc"; // Pointer to constant char
      break;
    case visitors::ResolvedType::TypeKind::Pointer:
      mangled << "P";
      // Recursively mangle the pointee type
      // This is a simplification - a real implementation would recurse
      mangled << "v"; // Assume void* for now
      break;
    case visitors::ResolvedType::TypeKind::Named: {
      std::string typeName = paramType->getName();
      mangled << typeName.length() << typeName;
      break;
    }
    default:
      mangled << "u"; // Unknown/unsupported type
      break;
    }
  }

  return mangled.str();
}

} // namespace LLVMUtils
} // namespace codegen
