#pragma once
#include <memory>
#include <string>
#include <unordered_map>

namespace visitors {

// Forward declaration
class ResolvedType;

/**
 * @brief Scope for type checking
 *
 * Maintains a symbol table for variables, functions, and types in the current
 * scope.
 */
class TypeScope : public std::enable_shared_from_this<TypeScope> {
public:
  TypeScope(std::shared_ptr<TypeScope> parent = nullptr) : parent_(parent) {}

  // Variable declarations
  void declareVariable(const std::string &name,
                       std::shared_ptr<ResolvedType> type);
  std::shared_ptr<ResolvedType> lookupVariable(const std::string &name) const;

  // Function declarations
  void declareFunction(const std::string &name,
                       std::shared_ptr<ResolvedType> type);
  std::shared_ptr<ResolvedType> lookupFunction(const std::string &name) const;

  // Type declarations
  void declareType(const std::string &name, std::shared_ptr<ResolvedType> type);
  std::shared_ptr<ResolvedType> lookupType(const std::string &name) const;

  // Create a new nested scope
  std::shared_ptr<TypeScope> createChildScope() const;

private:
  std::shared_ptr<TypeScope> parent_;
  std::unordered_map<std::string, std::shared_ptr<ResolvedType>> variables_;
  std::unordered_map<std::string, std::shared_ptr<ResolvedType>> functions_;
  std::unordered_map<std::string, std::shared_ptr<ResolvedType>> types_;
};

} // namespace visitors