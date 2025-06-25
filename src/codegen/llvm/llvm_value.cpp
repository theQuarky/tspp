#include "codegen/llvm/llvm_value.h"

namespace codegen {

LLVMValue::LLVMValue() : value_(nullptr), isLValue_(false) {}

LLVMValue::LLVMValue(llvm::Value *value,
                     std::shared_ptr<visitors::ResolvedType> type,
                     bool isLValue)
    : value_(value), type_(std::move(type)), isLValue_(isLValue) {}

LLVMValue LLVMValue::loadIfLValue(llvm::IRBuilder<> &builder) const {
  if (!isLValue_ || !value_) {
    return *this;
  }

  // Get the element type correctly
  llvm::Type *elementType = nullptr;
  llvm::PointerType *ptrType =
      llvm::dyn_cast<llvm::PointerType>(value_->getType());
  if (ptrType) {
    elementType = ptrType->getExtendedType();
  } else {
    // Fallback - though this shouldn't happen for proper lvalues
    elementType = llvm::Type::getInt8Ty(builder.getContext());
  }

  // Use the correct CreateLoad signature
  llvm::Value *loadedValue = builder.CreateLoad(elementType, value_, "load");
  return LLVMValue(loadedValue, type_, false);
}

} // namespace codegen