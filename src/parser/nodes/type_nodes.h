/*****************************************************************************
 * File: type_nodes.h
 * Description: AST node definitions for TSPP type system
 *****************************************************************************/

#pragma once
#include "base_node.h"
#include "core/common/common_types.h"
#include "tokens/token_type.h"
#include <memory>
#include <unordered_set>
#include <vector>

namespace nodes {

// Forward declarations
class ExpressionNode;
using ExpressionPtr = std::shared_ptr<ExpressionNode>;

/**
 * Base class for all type nodes in the AST
 */
class TypeNode : public BaseNode {
public:
  TypeNode(const core::SourceLocation &loc) : BaseNode(loc) {}
  virtual ~TypeNode() = default;

  // Type-specific methods
  virtual bool isVoid() const { return false; }
  virtual bool isPrimitive() const { return false; }
  virtual bool isPointer() const { return false; }
  virtual bool isArray() const { return false; }
  virtual bool isFunction() const { return false; }
  virtual bool isTemplate() const { return false; }

  virtual std::string toString() const = 0;
};

using TypePtr = std::shared_ptr<TypeNode>;

/**
 * Primitive type node (void, int, float, etc.)
 */
class PrimitiveTypeNode : public TypeNode {
public:
  PrimitiveTypeNode(tokens::TokenType type, const core::SourceLocation &loc)
      : TypeNode(loc), type_(type) {}

  tokens::TokenType getType() const { return type_; }
  bool isPrimitive() const override { return true; }
  bool isVoid() const override { return type_ == tokens::TokenType::VOID; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  };
  std::string toString() const override {
    switch (type_) {
    case tokens::TokenType::VOID:
      return "void";
    case tokens::TokenType::INT:
      return "int";
    case tokens::TokenType::FLOAT:
      return "float";
    case tokens::TokenType::BOOLEAN:
      return "boolean";
    case tokens::TokenType::STRING:
      return "string";
    default:
      return "unknown";
    }
  }

private:
  tokens::TokenType type_; // VOID, INT, FLOAT, etc.
};

/**
 * Named type node (user-defined types, type parameters)
 */
class NamedTypeNode : public TypeNode {
public:
  NamedTypeNode(const std::string &name, const core::SourceLocation &loc)
      : TypeNode(loc), name_(name) {}

  const std::string &getName() const { return name_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  };
  std::string toString() const override { return name_; }

private:
  std::string name_;
};

/**
 * Qualified type node (namespace.type)
 */
class QualifiedTypeNode : public TypeNode {
public:
  QualifiedTypeNode(std::vector<std::string> qualifiers,
                    const core::SourceLocation &loc)
      : TypeNode(loc), qualifiers_(std::move(qualifiers)) {}

  const std::vector<std::string> &getQualifiers() const { return qualifiers_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  };
  std::string toString() const override {
    if (qualifiers_.empty()) {
      return "";
    }

    std::string result = qualifiers_[0];
    for (size_t i = 1; i < qualifiers_.size(); i++) {
      result += '.';
      result += qualifiers_[i];
    }
    return result;
  }

private:
  std::vector<std::string> qualifiers_;
};

/**
 * Array type node (T[])
 */
class ArrayTypeNode : public TypeNode {
public:
  ArrayTypeNode(TypePtr elementType, ExpressionPtr size,
                const core::SourceLocation &loc)
      : TypeNode(loc), elementType_(std::move(elementType)),
        size_(std::move(size)) {}

  TypePtr getElementType() const { return elementType_; }
  ExpressionPtr getSize() const { return size_; }
  bool isArray() const override { return true; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

  std::string toString() const override {
    std::string result = elementType_->toString() + "[]";
    return result;
  }

private:
  TypePtr elementType_; // Type of array elements
  ExpressionPtr size_;  // Optional size expression
};

/**
 * Pointer type node (T@)
 */
class PointerTypeNode : public TypeNode {
public:
  enum class PointerKind {
    Raw,    // T@
    Safe,   // T@safe
    Unsafe, // T@unsafe
    Aligned // T@aligned(N)
  };

  PointerTypeNode(TypePtr baseType, PointerKind kind, ExpressionPtr alignment,
                  const core::SourceLocation &loc)
      : TypeNode(loc), baseType_(std::move(baseType)), kind_(kind),
        alignment_(std::move(alignment)) {}

  TypePtr getBaseType() const { return baseType_; }
  PointerKind getKind() const { return kind_; }
  ExpressionPtr getAlignment() const { return alignment_; }
  bool isPointer() const override { return true; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  };
  std::string toString() const override {
    std::ostringstream oss;
    oss << baseType_->toString() << "@";
    switch (kind_) {
    case PointerKind::Raw:
      break;
    case PointerKind::Safe:
      oss << "safe";
      break;
    case PointerKind::Unsafe:
      oss << "unsafe";
      break;
    case PointerKind::Aligned:
      oss << "aligned(" << alignment_ << ")";
      break;
    }
    return oss.str();
  }

private:
  TypePtr baseType_;        // Type being pointed to
  PointerKind kind_;        // Kind of pointer
  ExpressionPtr alignment_; // Alignment for aligned pointers
};

/**
 * Reference type node (T&)
 */
class ReferenceTypeNode : public TypeNode {
public:
  ReferenceTypeNode(TypePtr baseType, const core::SourceLocation &loc)
      : TypeNode(loc), baseType_(std::move(baseType)) {}

  TypePtr getBaseType() const { return baseType_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  };
  std::string toString() const override { return baseType_->toString() + "&"; }

private:
  TypePtr baseType_;
};

/**
 * Function type node (returns and parameters)
 */
class FunctionTypeNode : public TypeNode {
public:
  FunctionTypeNode(const std::vector<TypePtr> &paramTypes, TypePtr returnType,
                   const core::SourceLocation &location)
      : TypeNode(location), paramTypes_(paramTypes), returnType_(returnType) {}

  // Override visitor accept method
  TypePtr getReturnType() const { return returnType_; }
  const std::vector<TypePtr> &getParameterTypes() const { return paramTypes_; }
  bool isFunction() const override { return true; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  };
  std::string toString() const override {
    std::ostringstream oss;
    oss << "function (";

    for (size_t i = 0; i < paramTypes_.size(); ++i) {
      oss << paramTypes_[i]->toString();
      if (i < paramTypes_.size() - 1) {
        oss << ", ";
      }
    }

    oss << "): " << returnType_->toString();
    return oss.str();
  }

private:
  std::vector<TypePtr> paramTypes_;
  TypePtr returnType_;
};

/**
 * Template type node (Container<T>)
 */
class TemplateTypeNode : public TypeNode {
public:
  TemplateTypeNode(TypePtr baseType, std::vector<TypePtr> arguments,
                   const core::SourceLocation &loc)
      : TypeNode(loc), baseType_(std::move(baseType)),
        arguments_(std::move(arguments)) {}

  TypePtr getBaseType() const { return baseType_; }
  const std::vector<TypePtr> &getArguments() const { return arguments_; }
  bool isTemplate() const override { return true; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  };
  std::string toString() const override {
    std::ostringstream oss;
    oss << baseType_->toString() << "<";
    for (size_t i = 0; i < arguments_.size(); ++i) {
      oss << arguments_[i]->toString();
      if (i != arguments_.size() - 1) {
        oss << ", ";
      }
    }
    oss << ">";
    return oss.str();
  }

private:
  TypePtr baseType_;               // Template type being instantiated
  std::vector<TypePtr> arguments_; // Template arguments
};

/**
 * Smart pointer type node (#shared<T>, #unique<T>, #weak<T>)
 */
class SmartPointerTypeNode : public TypeNode {
public:
  enum class SmartPointerKind { Shared, Unique, Weak };

  SmartPointerTypeNode(TypePtr pointeeType, SmartPointerKind kind,
                       const core::SourceLocation &loc)
      : TypeNode(loc), pointeeType_(std::move(pointeeType)), kind_(kind) {}

  TypePtr getPointeeType() const { return pointeeType_; }
  SmartPointerKind getKind() const { return kind_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  };
  std::string toString() const override {
    std::string kindStr;
    switch (kind_) {
    case SmartPointerKind::Shared:
      kindStr = "#shared";
      break;
    case SmartPointerKind::Unique:
      kindStr = "#unique";
      break;
    case SmartPointerKind::Weak:
      kindStr = "#weak";
      break;
    }
    return kindStr + "<" + pointeeType_->toString() + ">";
  }

private:
  TypePtr pointeeType_;
  SmartPointerKind kind_;
};

/**
 * unioun type node (int | float | string)
 */
class UnionTypeNode : public TypeNode {
public:
  UnionTypeNode(TypePtr left, TypePtr right, const core::SourceLocation &loc)
      : TypeNode(loc), left_(std::move(left)), right_(std::move(right)) {}

  TypePtr getLeft() const { return left_; }
  TypePtr getRight() const { return right_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

  // Provide an implementation for the pure virtual function.
  std::string toString() const override {
    return left_->toString() + " | " + right_->toString();
  }

private:
  TypePtr left_;
  TypePtr right_;
};

class GenericParamNode : public nodes::TypeNode {
public:
  GenericParamNode(std::string name, std::vector<nodes::TypePtr> constraints,
                   const core::SourceLocation &loc)
      : TypeNode(loc), name_(std::move(name)),
        constraints_(std::move(constraints)) {}

  const std::string &getName() const { return name_; }
  const std::vector<nodes::TypePtr> &getConstraints() const {
    return constraints_;
  }

  bool isTemplate() const override { return true; }

  std::string toString() const override {
    std::string result = name_;
    if (!constraints_.empty()) {
      result += " extends ";
      for (size_t i = 0; i < constraints_.size(); ++i) {
        if (i > 0)
          result += " & ";
        result += constraints_[i]->toString();
      }
    }
    return result;
  }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::string name_;
  std::vector<nodes::TypePtr> constraints_;
};

// Define the BuiltinConstraintNode class
class BuiltinConstraintNode : public nodes::TypeNode {
public:
  BuiltinConstraintNode(std::string constraintName,
                        const core::SourceLocation &loc)
      : TypeNode(loc), constraintName_(std::move(constraintName)) {}

  const std::string &getConstraintName() const { return constraintName_; }

  std::string toString() const override { return constraintName_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::string constraintName_;
};

// Helper methods for checking valid constraint names
inline bool isValidBuiltinConstraint(const std::string &name) {
  static const std::unordered_set<std::string> builtinConstraints = {
      "number",  "comparable",    "equatable",
      "default", "constructible", "copyable"};
  return builtinConstraints.find(name) != builtinConstraints.end();
}

// Forward declarations for type visitors
class TypeVisitor {
public:
  virtual ~TypeVisitor() = default;
  virtual bool visitPrimitiveType(PrimitiveTypeNode *node) = 0;
  virtual bool visitNamedType(NamedTypeNode *node) = 0;
  virtual bool visitQualifiedType(QualifiedTypeNode *node) = 0;
  virtual bool visitArrayType(ArrayTypeNode *node) = 0;
  virtual bool visitPointerType(PointerTypeNode *node) = 0;
  virtual bool visitReferenceType(ReferenceTypeNode *node) = 0;
  virtual bool visitFunctionType(FunctionTypeNode *node) = 0;
  virtual bool visitTemplateType(TemplateTypeNode *node) = 0;
  virtual bool visitSmartPointerType(SmartPointerTypeNode *node) = 0;
};

} // namespace nodes