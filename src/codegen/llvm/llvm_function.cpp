#include "codegen/llvm/llvm_function.h"

namespace codegen {

LLVMFunction::LLVMFunction(LLVMContext &context, llvm::Function *func,
                           std::shared_ptr<visitors::ResolvedType> returnType)
    : context_(context), function_(func), returnType_(std::move(returnType)) {
  // Create an initial scope
  enterScope();
}

void LLVMFunction::enterScope() { scopes_.push_back(Scope()); }

void LLVMFunction::exitScope() {
  if (scopes_.size() > 1) { // Always keep at least one scope
    scopes_.pop_back();
  }
}

void LLVMFunction::declareVariable(const std::string &name,
                                   const LLVMValue &value) {
  if (!scopes_.empty()) {
    scopes_.back().variables[name] = value;
  }
}

LLVMValue LLVMFunction::getVariable(const std::string &name) const {
  // Search scopes from innermost to outermost
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto varIt = it->variables.find(name);
    if (varIt != it->variables.end()) {
      return varIt->second;
    }
  }

  // Variable not found
  return LLVMValue();
}

llvm::BasicBlock *LLVMFunction::createBasicBlock(const std::string &name) {
  return llvm::BasicBlock::Create(context_.getContext(), name, function_);
}

void LLVMFunction::setInsertPoint(llvm::BasicBlock *block) {
  context_.getBuilder().SetInsertPoint(block);
}

void LLVMFunction::mapParameters(const std::vector<std::string> &paramNames) {
  // Create entry block for parameter allocations
  llvm::BasicBlock *entryBlock = &function_->getEntryBlock();
  llvm::IRBuilder<> tempBuilder(entryBlock, entryBlock->begin());

  unsigned int paramIndex = 0;
  for (auto &arg : function_->args()) {
    if (paramIndex < paramNames.size()) {
      // Create an allocation for the parameter
      llvm::AllocaInst *alloca = tempBuilder.CreateAlloca(
          arg.getType(), nullptr, paramNames[paramIndex]);

      // Store the parameter value to the allocation
      context_.getBuilder().CreateStore(&arg, alloca);

      // Create a placeholder type for now
      auto paramType = std::make_shared<visitors::ResolvedType>(
          visitors::ResolvedType::Int());

      // Declare the variable in the current scope
      declareVariable(paramNames[paramIndex],
                      LLVMValue(alloca, paramType, true));
    }
    ++paramIndex;
  }
}

} // namespace codegen