/*****************************************************************************
 * File: declaration_nodes.h
 * Description: AST node definitions for declarations in TSPP language
 *****************************************************************************/

#pragma once
#include "base_node.h"
#include "expression_nodes.h"
#include "tokens/token_type.h"
#include "type_nodes.h"
#include <memory>
#include <vector>

namespace nodes {

// Forward declaration for BlockNode - will be implemented later
class BlockNode;
using BlockPtr = std::shared_ptr<BlockNode>;
using AttributePtr = std::shared_ptr<AttributeNode>;

/**
 * Base class for all declaration nodes
 */
class DeclarationNode : public BaseNode {
public:
  DeclarationNode(const std::string &name, const core::SourceLocation &loc)
      : BaseNode(loc), name_(name) {}

  virtual ~DeclarationNode() = default;

  const std::string &getName() const { return name_; }
  const std::vector<AttributePtr> &getAttributes() const { return attributes_; }
  void addAttribute(AttributePtr attr) {
    attributes_.push_back(std::move(attr));
  }

protected:
  std::string name_;                     // Declaration name
  std::vector<AttributePtr> attributes_; // Attributes (#stack, #inline, etc.)
};

using DeclPtr = std::shared_ptr<DeclarationNode>;

/**
 * Variable declaration node (let x: int = 42)
 */
class VarDeclNode : public DeclarationNode {
public:
  VarDeclNode(const std::string &name, TypePtr type, ExpressionPtr initializer,
              tokens::TokenType storageClass, bool isConst,
              const core::SourceLocation &loc)
      : DeclarationNode(name, loc), type_(std::move(type)),
        initializer_(std::move(initializer)), storageClass_(storageClass),
        isConst_(isConst) {}

  TypePtr getType() const { return type_; }
  ExpressionPtr getInitializer() const { return initializer_; }
  tokens::TokenType getStorageClass() const { return storageClass_; }
  bool isConst() const { return isConst_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  TypePtr type_;                   // Variable type (can be nullptr if inferred)
  ExpressionPtr initializer_;      // Initial value (can be nullptr)
  tokens::TokenType storageClass_; // #stack, #heap, #static
  bool isConst_;                   // Whether declared with 'const'
};

/**
 * Function parameter declaration
 */
class ParameterNode : public DeclarationNode {
public:
  ParameterNode(const std::string &name, TypePtr type,
                ExpressionPtr defaultValue, bool isRef, bool isConst,
                const core::SourceLocation &loc)
      : DeclarationNode(name, loc), type_(std::move(type)),
        defaultValue_(std::move(defaultValue)), isRef_(isRef),
        isConst_(isConst) {}

  TypePtr getType() const { return type_; }
  ExpressionPtr getDefaultValue() const { return defaultValue_; }
  bool isRef() const { return isRef_; }
  bool isConst() const { return isConst_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  TypePtr type_;               // Parameter type
  ExpressionPtr defaultValue_; // Default value (can be nullptr)
  bool isRef_;                 // Whether parameter is by reference
  bool isConst_;               // Whether parameter is const
};

using ParamPtr = std::shared_ptr<ParameterNode>;

/**
 * Function declaration node
 */
class FunctionDeclNode : public DeclarationNode {
public:
  FunctionDeclNode(const std::string &name, std::vector<ParamPtr> params,
                   TypePtr returnType,
                   std::vector<TypePtr> throwsTypes,         // Add throws types
                   std::vector<tokens::TokenType> modifiers, // Add modifiers
                   BlockPtr body, bool isAsync, const core::SourceLocation &loc)
      : DeclarationNode(name, loc), parameters_(std::move(params)),
        returnType_(std::move(returnType)),
        throwsTypes_(std::move(throwsTypes)), modifiers_(std::move(modifiers)),
        body_(std::move(body)), isAsync_(isAsync) {}

  const std::vector<ParamPtr> &getParameters() const { return parameters_; }
  TypePtr getReturnType() const { return returnType_; }
  const std::vector<TypePtr> &getThrowsTypes() const {
    return throwsTypes_;
  } // Add getter
  const std::vector<tokens::TokenType> &getModifiers() const {
    return modifiers_;
  }
  BlockPtr getBody() const { return body_; }
  bool isAsync() const { return isAsync_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::vector<ParamPtr> parameters_;
  TypePtr returnType_;
  std::vector<TypePtr> throwsTypes_;         // Add throws types member
  std::vector<tokens::TokenType> modifiers_; // Store function modifiers
  BlockPtr body_;
  bool isAsync_;
};

/**
 * Generic Function declaration node
 */
class GenericFunctionDeclNode : public FunctionDeclNode {
public:
  GenericFunctionDeclNode(
      const std::string &name, std::vector<TypePtr> genericParams,
      std::vector<ParamPtr> params, TypePtr returnType,
      std::vector<std::pair<std::string, TypePtr>> constraints,
      std::vector<TypePtr> throwsTypes,         // Add throws types
      std::vector<tokens::TokenType> modifiers, // Add modifiers
      BlockPtr body, bool isAsync, const core::SourceLocation &loc)
      : FunctionDeclNode(name, std::move(params), std::move(returnType),
                         std::move(throwsTypes), // Pass throws types to base
                         std::move(modifiers), std::move(body), isAsync, loc),
        genericParams_(std::move(genericParams)),
        constraints_(std::move(constraints)) {}

  const std::vector<TypePtr> &getGenericParams() const {
    return genericParams_;
  }
  const std::vector<std::pair<std::string, TypePtr>> &getConstraints() const {
    return constraints_;
  }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::vector<TypePtr> genericParams_;
  std::vector<std::pair<std::string, TypePtr>> constraints_;
};

/**
 * Class declaration node
 */
class ClassDeclNode : public DeclarationNode {
public:
  ClassDeclNode(const std::string &name,
                std::vector<tokens::TokenType> classModifiers,
                TypePtr baseClass, std::vector<TypePtr> interfaces,
                std::vector<DeclPtr> members, const core::SourceLocation &loc)
      : DeclarationNode(name, loc), classModifiers_(std::move(classModifiers)),
        baseClass_(std::move(baseClass)), interfaces_(std::move(interfaces)),
        members_(std::move(members)) {}

  const std::vector<tokens::TokenType> &getClassModifiers() const {
    return classModifiers_;
  }
  TypePtr getBaseClass() const { return baseClass_; }
  const std::vector<TypePtr> &getInterfaces() const { return interfaces_; }
  const std::vector<DeclPtr> &getMembers() const { return members_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::vector<tokens::TokenType>
      classModifiers_;              // #aligned, #packed, #abstract
  TypePtr baseClass_;               // Base class (can be nullptr)
  std::vector<TypePtr> interfaces_; // Implemented interfaces
  std::vector<DeclPtr> members_;    // Class members
};

class GenericClassDeclNode : public ClassDeclNode {
public:
  GenericClassDeclNode(const std::string &name,
                       std::vector<tokens::TokenType> classModifiers,
                       TypePtr baseClass, std::vector<TypePtr> interfaces,
                       std::vector<DeclPtr> members,
                       std::vector<TypePtr> genericParams,
                       const core::SourceLocation &loc)
      : ClassDeclNode(name, std::move(classModifiers), std::move(baseClass),
                      std::move(interfaces), std::move(members), loc),
        genericParams_(std::move(genericParams)) {}

  const std::vector<TypePtr> &getGenericParams() const {
    return genericParams_;
  }

private:
  std::vector<TypePtr> genericParams_;
};

class ConstructorDeclNode : public DeclarationNode {
public:
  ConstructorDeclNode(tokens::TokenType accessModifier,
                      std::vector<ParamPtr> parameters, BlockPtr body,
                      const core::SourceLocation &loc)
      : DeclarationNode("constructor", loc), accessModifier_(accessModifier),
        parameters_(std::move(parameters)), body_(std::move(body)) {}

  tokens::TokenType getAccessModifier() const { return accessModifier_; }
  const std::vector<ParamPtr> &getParameters() const { return parameters_; }
  BlockPtr getBody() const { return body_; }

  bool accept(interface::BaseInterface *visitor) override {
    // You might implement a specific visit method, e.g.
    // visitor->visitConstructor(this)
    return visitor->visitParse();
  }

private:
  tokens::TokenType accessModifier_; // e.g. PUBLIC, PRIVATE, PROTECTED
  std::vector<ParamPtr> parameters_; // List of parameters
  BlockPtr body_;                    // The constructor body (block)
};

class MethodDeclNode : public DeclarationNode {
public:
  MethodDeclNode(const std::string &methodName,
                 tokens::TokenType accessModifier,
                 std::vector<ParamPtr> parameters, TypePtr returnType,
                 std::vector<TypePtr> throwsTypes,
                 std::vector<tokens::TokenType> modifiers, BlockPtr body,
                 const core::SourceLocation &loc)
      : DeclarationNode(methodName, loc), accessModifier_(accessModifier),
        parameters_(std::move(parameters)), returnType_(std::move(returnType)),
        throwsTypes_(std::move(throwsTypes)), modifiers_(std::move(modifiers)),
        body_(std::move(body)) {}

  tokens::TokenType getAccessModifier() const { return accessModifier_; }
  const std::vector<ParamPtr> &getParameters() const { return parameters_; }
  TypePtr getReturnType() const { return returnType_; }
  const std::vector<TypePtr> &getThrowsTypes() const { return throwsTypes_; }
  const std::vector<tokens::TokenType> &getModifiers() const {
    return modifiers_;
  }
  BlockPtr getBody() const { return body_; }

  bool accept(interface::BaseInterface *visitor) override {
    // e.g. visitor->visitMethod(this);
    return visitor->visitParse();
  }

private:
  tokens::TokenType accessModifier_;
  std::vector<ParamPtr> parameters_;
  TypePtr returnType_;
  std::vector<TypePtr> throwsTypes_;
  std::vector<tokens::TokenType> modifiers_; // e.g. #inline, #virtual, #unsafe
  BlockPtr body_;
};

class FieldDeclNode : public DeclarationNode {
public:
  FieldDeclNode(const std::string &name, tokens::TokenType accessModifier,
                bool isConst, TypePtr type, ExpressionPtr initializer,
                const core::SourceLocation &loc)
      : DeclarationNode(name, loc), accessModifier_(accessModifier),
        isConst_(isConst), type_(std::move(type)),
        initializer_(std::move(initializer)) {}

  tokens::TokenType getAccessModifier() const { return accessModifier_; }
  bool isConst() const { return isConst_; }
  TypePtr getType() const { return type_; }
  ExpressionPtr getInitializer() const { return initializer_; }

  bool accept(interface::BaseInterface *visitor) override {
    // e.g. visitor->visitField(this);
    return visitor->visitParse();
  }

private:
  tokens::TokenType accessModifier_; // e.g. PUBLIC, PRIVATE, PROTECTED
  bool isConst_;                     // declared with 'const' vs 'let'
  TypePtr type_;                     // optional if type is inferred
  ExpressionPtr initializer_;        // e.g. "= 42"
};

enum class PropertyKind { Getter, Setter };

class PropertyDeclNode : public DeclarationNode {
public:
  // For a getter:
  PropertyDeclNode(const std::string &propName,
                   tokens::TokenType accessModifier, PropertyKind kind,
                   TypePtr propertyType, // e.g. :int
                   BlockPtr body, const core::SourceLocation &loc)
      : DeclarationNode(propName, loc), accessModifier_(accessModifier),
        kind_(kind), propertyType_(std::move(propertyType)),
        body_(std::move(body)) {}

  // For a setter (where you'd have a parameter):
  // You could add another constructor or store a ParameterNode for "value".

  tokens::TokenType getAccessModifier() const { return accessModifier_; }
  PropertyKind getKind() const { return kind_; }
  TypePtr getPropertyType() const { return propertyType_; }
  BlockPtr getBody() const { return body_; }

  bool accept(interface::BaseInterface *visitor) override {
    // e.g. visitor->visitProperty(this);
    return visitor->visitParse();
  }

private:
  tokens::TokenType accessModifier_;
  PropertyKind kind_;    // Getter vs. Setter
  TypePtr propertyType_; // For getter (e.g. ": int")
  BlockPtr body_;
};

/**
 * Namespace declaration node
 */
class NamespaceDeclNode : public DeclarationNode {
public:
  NamespaceDeclNode(const std::string &name, std::vector<DeclPtr> declarations,
                    const core::SourceLocation &loc)
      : DeclarationNode(name, loc), declarations_(std::move(declarations)) {}

  const std::vector<DeclPtr> &getDeclarations() const { return declarations_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::vector<DeclPtr> declarations_; // Declarations inside the namespace
};

/**
 * Enum member node
 */
class EnumMemberNode : public DeclarationNode {
public:
  EnumMemberNode(const std::string &name,
                 ExpressionPtr value, // Optional explicit value
                 const core::SourceLocation &loc)
      : DeclarationNode(name, loc), value_(std::move(value)) {}

  ExpressionPtr getValue() const { return value_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  ExpressionPtr value_; // Optional explicit value for the enum member
};

using EnumMemberPtr = std::shared_ptr<EnumMemberNode>;

/**
 * Enum declaration node
 */
class EnumDeclNode : public DeclarationNode {
public:
  EnumDeclNode(
      const std::string &name,
      TypePtr underlyingType, // Optional underlying type (e.g., int, string)
      std::vector<EnumMemberPtr> members, const core::SourceLocation &loc)
      : DeclarationNode(name, loc), underlyingType_(std::move(underlyingType)),
        members_(std::move(members)) {}

  TypePtr getUnderlyingType() const { return underlyingType_; }
  const std::vector<EnumMemberPtr> &getMembers() const { return members_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  TypePtr underlyingType_;             // Optional underlying type
  std::vector<EnumMemberPtr> members_; // Enum members
};

/**
 * Method signature node (used in interfaces)
 */
class MethodSignatureNode : public DeclarationNode {
public:
  MethodSignatureNode(const std::string &name, tokens::TokenType accessModifier,
                      std::vector<ParamPtr> parameters, TypePtr returnType,
                      std::vector<TypePtr> throwsTypes,
                      const core::SourceLocation &loc)
      : DeclarationNode(name, loc), accessModifier_(accessModifier),
        parameters_(std::move(parameters)), returnType_(std::move(returnType)),
        throwsTypes_(std::move(throwsTypes)) {}

  tokens::TokenType getAccessModifier() const { return accessModifier_; }
  const std::vector<ParamPtr> &getParameters() const { return parameters_; }
  TypePtr getReturnType() const { return returnType_; }
  const std::vector<TypePtr> &getThrowsTypes() const { return throwsTypes_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  tokens::TokenType accessModifier_; // public, private, protected
  std::vector<ParamPtr> parameters_; // Method parameters
  TypePtr returnType_;               // Return type
  std::vector<TypePtr> throwsTypes_; // Exception types
};

/**
 * Property signature node (used in interfaces)
 */
class PropertySignatureNode : public DeclarationNode {
public:
  PropertySignatureNode(const std::string &name,
                        tokens::TokenType accessModifier, TypePtr type,
                        bool hasGetter, bool hasSetter,
                        const core::SourceLocation &loc)
      : DeclarationNode(name, loc), accessModifier_(accessModifier),
        type_(std::move(type)), hasGetter_(hasGetter), hasSetter_(hasSetter) {}

  tokens::TokenType getAccessModifier() const { return accessModifier_; }
  TypePtr getType() const { return type_; }
  bool hasGetter() const { return hasGetter_; }
  bool hasSetter() const { return hasSetter_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  tokens::TokenType accessModifier_; // public, private, protected
  TypePtr type_;                     // Property type
  bool hasGetter_;                   // Whether property has a getter
  bool hasSetter_;                   // Whether property has a setter
};

/**
 * Interface declaration node
 */
class InterfaceDeclNode : public DeclarationNode {
public:
  InterfaceDeclNode(const std::string &name,
                    std::vector<TypePtr> extendedInterfaces,
                    std::vector<DeclPtr> members,
                    bool isZeroCast, // #zerocast attribute
                    const core::SourceLocation &loc)
      : DeclarationNode(name, loc),
        extendedInterfaces_(std::move(extendedInterfaces)),
        members_(std::move(members)), isZeroCast_(isZeroCast) {}

  const std::vector<TypePtr> &getExtendedInterfaces() const {
    return extendedInterfaces_;
  }

  const std::vector<DeclPtr> &getMembers() const { return members_; }

  bool isZeroCast() const { return isZeroCast_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::vector<TypePtr> extendedInterfaces_; // Interfaces that this one extends
  std::vector<DeclPtr>
      members_;     // Interface members (method signatures, properties)
  bool isZeroCast_; // Whether this interface has #zerocast attribute
};

/**
 * Generic interface declaration node
 */
class GenericInterfaceDeclNode : public InterfaceDeclNode {
public:
  GenericInterfaceDeclNode(const std::string &name,
                           std::vector<TypePtr> extendedInterfaces,
                           std::vector<DeclPtr> members, bool isZeroCast,
                           std::vector<TypePtr> genericParams,
                           const core::SourceLocation &loc)
      : InterfaceDeclNode(name, std::move(extendedInterfaces),
                          std::move(members), isZeroCast, loc),
        genericParams_(std::move(genericParams)) {}

  const std::vector<TypePtr> &getGenericParams() const {
    return genericParams_;
  }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::vector<TypePtr> genericParams_; // Generic type parameters
};

/**
 * Type alias (typedef) declaration node
 */
class TypedefDeclNode : public DeclarationNode {
public:
  TypedefDeclNode(const std::string &name, TypePtr aliasedType,
                  const core::SourceLocation &loc)
      : DeclarationNode(name, loc), aliasedType_(std::move(aliasedType)) {}

  TypePtr getAliasedType() const { return aliasedType_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  TypePtr aliasedType_; // Type being aliased
};

// Forward declarations for declaration visitors
class DeclVisitor {
public:
  virtual ~DeclVisitor() = default;
  virtual bool visitVarDecl(VarDeclNode *node) = 0;
  virtual bool visitParameter(ParameterNode *node) = 0;
  virtual bool visitFunction(FunctionDeclNode *node) = 0;
  virtual bool visitAttribute(AttributeNode *node) = 0;

  // New visitor methods for namespaces, interfaces, and enums
  virtual bool visitNamespace(NamespaceDeclNode *node) = 0;
  virtual bool visitInterface(InterfaceDeclNode *node) = 0;
  virtual bool visitGenericInterface(GenericInterfaceDeclNode *node) = 0;
  virtual bool visitMethodSignature(MethodSignatureNode *node) = 0;
  virtual bool visitPropertySignature(PropertySignatureNode *node) = 0;
  virtual bool visitEnum(EnumDeclNode *node) = 0;
  virtual bool visitEnumMember(EnumMemberNode *node) = 0;
  virtual bool visitTypedef(TypedefDeclNode *node) = 0;
};

} // namespace nodes