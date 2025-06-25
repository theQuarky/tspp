#include "type_scope.h"
#include "resolved_type.h"

namespace visitors {

// Variable declaration and lookup methods
void TypeScope::declareVariable(const std::string &name,
                                std::shared_ptr<ResolvedType> type) {
  variables_[name] = std::move(type);
}

std::shared_ptr<ResolvedType>
TypeScope::lookupVariable(const std::string &name) const {
  // Try to find in current scope
  auto it = variables_.find(name);
  if (it != variables_.end()) {
    return it->second;
  }

  // If not found and we have a parent scope, try looking there
  if (parent_) {
    return parent_->lookupVariable(name);
  }

  // Not found anywhere
  return nullptr;
}

// Function declaration and lookup methods
void TypeScope::declareFunction(const std::string &name,
                                std::shared_ptr<ResolvedType> type) {
  functions_[name] = std::move(type);
}

std::shared_ptr<ResolvedType>
TypeScope::lookupFunction(const std::string &name) const {
  // Try to find in current scope
  auto it = functions_.find(name);
  if (it != functions_.end()) {
    return it->second;
  }

  // If not found and we have a parent scope, try looking there
  if (parent_) {
    return parent_->lookupFunction(name);
  }

  // Not found anywhere
  return nullptr;
}

// Type declaration and lookup methods
void TypeScope::declareType(const std::string &name,
                            std::shared_ptr<ResolvedType> type) {
  types_[name] = std::move(type);
}

std::shared_ptr<ResolvedType>
TypeScope::lookupType(const std::string &name) const {
  // Try to find in current scope
  auto it = types_.find(name);
  if (it != types_.end()) {
    return it->second;
  }

  // If not found and we have a parent scope, try looking there
  if (parent_) {
    return parent_->lookupType(name);
  }

  // Not found anywhere
  return nullptr;
}

// Create a new scope with this scope as parent
std::shared_ptr<TypeScope> TypeScope::createChildScope() const {
  return std::make_shared<TypeScope>(
      const_cast<TypeScope *>(this)->shared_from_this());
}

} // namespace visitors