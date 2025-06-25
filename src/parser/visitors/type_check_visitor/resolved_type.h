#pragma once
#include <memory>
#include <string>
#include <vector>

namespace visitors {

/**
 * @brief Represents a resolved type in the type system
 *
 * This class encapsulates all type information for a resolved type,
 * including its kind, underlying type details, and any generic parameters.
 */
class ResolvedType {
public:
  // Basic type kinds
  enum class TypeKind {
    Void,
    Int,
    Float,
    Bool,
    String,
    Named,     // User-defined types
    Array,     // Array types
    Pointer,   // Pointer types
    Reference, // Reference types
    Function,  // Function types
    Smart,     // Smart pointer types
    Union,     // Union types
    Template,  // Template specialization
    Error      // Error or unknown type
  };

  // Smart pointer kinds
  enum class SmartKind { Shared, Unique, Weak };

  // Constructors for different kinds of types
  static ResolvedType Void();
  static ResolvedType Int();
  static ResolvedType Float();
  static ResolvedType Bool();
  static ResolvedType String();
  static ResolvedType Named(const std::string &name);
  static ResolvedType Array(std::shared_ptr<ResolvedType> elementType);
  static ResolvedType Pointer(std::shared_ptr<ResolvedType> pointeeType,
                              bool isUnsafe = false);
  static ResolvedType Reference(std::shared_ptr<ResolvedType> refType);
  static ResolvedType
  Function(std::shared_ptr<ResolvedType> returnType,
           std::vector<std::shared_ptr<ResolvedType>> paramTypes);
  static ResolvedType Smart(std::shared_ptr<ResolvedType> pointeeType,
                            SmartKind kind);
  static ResolvedType Union(std::shared_ptr<ResolvedType> left,
                            std::shared_ptr<ResolvedType> right);
  static ResolvedType Template(const std::string &name,
                               std::vector<std::shared_ptr<ResolvedType>> args);
  static ResolvedType Error();

  // Type comparison and checking
  bool isAssignableTo(const ResolvedType &other) const;
  bool isImplicitlyConvertibleTo(const ResolvedType &other) const;
  bool isExplicitlyConvertibleTo(const ResolvedType &other) const;
  bool equals(const ResolvedType &other) const;

  // Accessors
  TypeKind getKind() const { return kind_; }
  const std::string &getName() const { return name_; }
  std::shared_ptr<ResolvedType> getElementType() const { return elementType_; }
  std::shared_ptr<ResolvedType> getPointeeType() const { return pointeeType_; }
  std::shared_ptr<ResolvedType> getReturnType() const { return returnType_; }
  const std::vector<std::shared_ptr<ResolvedType>> &getParameterTypes() const {
    return paramTypes_;
  }
  SmartKind getSmartKind() const { return smartKind_; }
  std::shared_ptr<ResolvedType> getLeftType() const { return leftType_; }
  std::shared_ptr<ResolvedType> getRightType() const { return rightType_; }
  const std::vector<std::shared_ptr<ResolvedType>> &getTemplateArgs() const {
    return templateArgs_;
  }
  bool isUnsafe() const { return isUnsafe_; }

  // String representation
  std::string toString() const;

private:
  ResolvedType(TypeKind kind) : kind_(kind), isUnsafe_(false) {}

  TypeKind kind_;
  std::string name_;                                      // For named types
  std::shared_ptr<ResolvedType> elementType_;             // For array types
  std::shared_ptr<ResolvedType> pointeeType_;             // For pointer types
  std::shared_ptr<ResolvedType> returnType_;              // For function types
  std::vector<std::shared_ptr<ResolvedType>> paramTypes_; // For function types
  SmartKind smartKind_;                     // For smart pointer types
  std::shared_ptr<ResolvedType> leftType_;  // For union types
  std::shared_ptr<ResolvedType> rightType_; // For union types
  std::vector<std::shared_ptr<ResolvedType>>
      templateArgs_; // For template types
  bool isUnsafe_;    // For unsafe pointers
};

} // namespace visitors