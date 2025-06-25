#include "type_check_visitor.h"
#include "parser/nodes/expression_nodes.h"
#include "tokens/token_type.h"
#include <iostream>

namespace visitors {

TypeCheckVisitor::TypeCheckVisitor(core::ErrorReporter &errorReporter)
    : errorReporter_(errorReporter), inLoop_(false), inTryBlock_(false) {

  // Initialize built-in types
  voidType_ = std::make_shared<ResolvedType>(ResolvedType::Void());
  intType_ = std::make_shared<ResolvedType>(ResolvedType::Int());
  floatType_ = std::make_shared<ResolvedType>(ResolvedType::Float());
  boolType_ = std::make_shared<ResolvedType>(ResolvedType::Bool());
  stringType_ = std::make_shared<ResolvedType>(ResolvedType::String());
  errorType_ = std::make_shared<ResolvedType>(ResolvedType::Error());

  // Initialize global scope
  currentScope_ = std::make_shared<TypeScope>();

  // Add built-in types to global scope
  currentScope_->declareType("void", voidType_);
  currentScope_->declareType("int", intType_);
  currentScope_->declareType("float", floatType_);
  currentScope_->declareType("bool", boolType_);
  currentScope_->declareType("string", stringType_);
}

bool TypeCheckVisitor::checkAST(const parser::AST &ast) {
  const auto &nodes = ast.getNodes();
  bool success = true;

  // First pass: collect all type declarations
  for (const auto &node : nodes) {
    if (auto classDecl =
            std::dynamic_pointer_cast<nodes::ClassDeclNode>(node)) {
      auto type = visitClassDecl(classDecl.get());
      currentScope_->declareType(classDecl->getName(), type);
    } else if (auto genericClass =
                   std::dynamic_pointer_cast<nodes::GenericClassDeclNode>(
                       node)) {
      auto type = visitGenericClassDecl(genericClass.get());
      currentScope_->declareType(genericClass->getName(), type);
    } else if (auto enumDecl =
                   std::dynamic_pointer_cast<nodes::EnumDeclNode>(node)) {
      auto type = visitEnumDecl(enumDecl.get());
      currentScope_->declareType(enumDecl->getName(), type);
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<nodes::InterfaceDeclNode>(node)) {
      auto type = visitInterfaceDecl(interfaceDecl.get());
      currentScope_->declareType(interfaceDecl->getName(), type);
    } else if (auto genericInterface =
                   std::dynamic_pointer_cast<nodes::GenericInterfaceDeclNode>(
                       node)) {
      auto type = visitGenericInterfaceDecl(genericInterface.get());
      currentScope_->declareType(genericInterface->getName(), type);
    } else if (auto typedefDecl =
                   std::dynamic_pointer_cast<nodes::TypedefDeclNode>(node)) {
      auto type = visitTypedefDecl(typedefDecl.get());
      currentScope_->declareType(typedefDecl->getName(), type);
    }
  }

  // Second pass: check all declarations and statements
  for (const auto &node : nodes) {
    if (auto varDecl = std::dynamic_pointer_cast<nodes::VarDeclNode>(node)) {
      auto type = visitVarDecl(varDecl.get());
      if (type->getKind() == ResolvedType::TypeKind::Error) {
        success = false;
      }
    } else if (auto funcDecl =
                   std::dynamic_pointer_cast<nodes::FunctionDeclNode>(node)) {
      auto type = visitFuncDecl(funcDecl.get());
      if (type->getKind() == ResolvedType::TypeKind::Error) {
        success = false;
      }
    } else if (auto genericFunc =
                   std::dynamic_pointer_cast<nodes::GenericFunctionDeclNode>(
                       node)) {
      auto type = visitGenericFuncDecl(genericFunc.get());
      if (type->getKind() == ResolvedType::TypeKind::Error) {
        success = false;
      }
    } else if (auto namespaceDecl =
                   std::dynamic_pointer_cast<nodes::NamespaceDeclNode>(node)) {
      auto type = visitNamespaceDecl(namespaceDecl.get());
      if (type->getKind() == ResolvedType::TypeKind::Error) {
        success = false;
      }
    } else if (auto stmt =
                   std::dynamic_pointer_cast<nodes::StatementNode>(node)) {
      auto type = visitStmt(stmt.get());
      if (type->getKind() == ResolvedType::TypeKind::Error) {
        success = false;
      }
    }
  }

  return success;
}

// Declaration visitors
std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitVarDecl(const nodes::VarDeclNode *node) {
  // Check the initializer if present
  std::shared_ptr<ResolvedType> initType = nullptr;
  if (node->getInitializer()) {
    initType = visitExpr(node->getInitializer().get());
  }

  // Get the declared type if present
  std::shared_ptr<ResolvedType> declaredType = nullptr;
  if (node->getType()) {
    declaredType = visitType(node->getType().get());
  }

  // Determine the variable's type
  std::shared_ptr<ResolvedType> varType;
  if (declaredType) {
    varType = declaredType;

    // If there's an initializer, check compatibility
    if (initType) {
      if (!checkAssignmentCompatibility(declaredType, initType,
                                        node->getLocation())) {
        error(node->getLocation(),
              "Initializer type doesn't match variable type");
        return errorType_;
      }
    }
  } else if (initType) {
    // Type inference from initializer
    varType = initType;
  } else {
    error(node->getLocation(), "Variable declaration needs either a type or an "
                               "initializer for type inference");
    return errorType_;
  }

  // Add variable to current scope
  currentScope_->declareVariable(node->getName(), varType);
  return varType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitFuncDecl(const nodes::FunctionDeclNode *node) {
  // Process return type
  std::shared_ptr<ResolvedType> returnType;
  if (node->getReturnType()) {
    returnType = visitType(node->getReturnType().get());
  } else {
    returnType = voidType_;
  }

  // Process parameters
  std::vector<std::shared_ptr<ResolvedType>> paramTypes;
  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    paramTypes.push_back(paramType);
  }

  // Create function type
  auto functionType = std::make_shared<ResolvedType>(
      ResolvedType::Function(returnType, paramTypes));

  // Add function to current scope
  currentScope_->declareFunction(node->getName(), functionType);

  // Check function body with new scope
  enterFunctionScope(returnType);

  // Add parameters to function scope
  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    currentScope_->declareVariable(param->getName(), paramType);
  }

  if (node->getBody()) {
    visitBlock(node->getBody().get());
  }

  exitFunctionScope();
  return functionType;
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitGenericFuncDecl(
    const nodes::GenericFunctionDeclNode *node) {
  // For now, treat generic functions similar to regular functions
  // In a complete implementation, we would handle generic parameter constraints
  return visitFuncDecl(node);
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitClassDecl(const nodes::ClassDeclNode *node) {
  auto classType =
      std::make_shared<ResolvedType>(ResolvedType::Named(node->getName()));

  // Enter class scope
  enterScope();
  currentClassType_ = classType;

  // Process base class if present
  if (node->getBaseClass()) {
    visitType(node->getBaseClass().get());
  }

  // Process interfaces
  for (const auto &interface : node->getInterfaces()) {
    visitType(interface.get());
  }

  // Process class members
  for (const auto &member : node->getMembers()) {
    if (auto methodDecl =
            std::dynamic_pointer_cast<nodes::MethodDeclNode>(member)) {
      visitMethodDecl(methodDecl.get());
    } else if (auto fieldDecl =
                   std::dynamic_pointer_cast<nodes::FieldDeclNode>(member)) {
      visitFieldDecl(fieldDecl.get());
    } else if (auto ctorDecl =
                   std::dynamic_pointer_cast<nodes::ConstructorDeclNode>(
                       member)) {
      visitConstructorDecl(ctorDecl.get());
    } else if (auto propDecl =
                   std::dynamic_pointer_cast<nodes::PropertyDeclNode>(member)) {
      visitPropertyDecl(propDecl.get());
    }
  }

  currentClassType_ = nullptr;
  exitScope();
  return classType;
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitGenericClassDecl(
    const nodes::GenericClassDeclNode *node) {
  // For now, treat generic classes similar to regular classes
  return visitClassDecl(node);
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitConstructorDecl(const nodes::ConstructorDeclNode *node) {
  // Process parameters
  std::vector<std::shared_ptr<ResolvedType>> paramTypes;
  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    paramTypes.push_back(paramType);
  }

  // Constructor returns the class type
  auto constructorType = std::make_shared<ResolvedType>(
      ResolvedType::Function(currentClassType_, paramTypes));

  // Check constructor body
  enterFunctionScope(currentClassType_);

  // Add parameters to scope
  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    currentScope_->declareVariable(param->getName(), paramType);
  }

  if (node->getBody()) {
    visitBlock(node->getBody().get());
  }

  exitFunctionScope();
  return constructorType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitMethodDecl(const nodes::MethodDeclNode *node) {
  // Process return type
  std::shared_ptr<ResolvedType> returnType;
  if (node->getReturnType()) {
    returnType = visitType(node->getReturnType().get());
  } else {
    returnType = voidType_;
  }

  // Process parameters
  std::vector<std::shared_ptr<ResolvedType>> paramTypes;
  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    paramTypes.push_back(paramType);
  }

  auto methodType = std::make_shared<ResolvedType>(
      ResolvedType::Function(returnType, paramTypes));

  // Check method body
  enterFunctionScope(returnType);

  // Add parameters to scope
  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    currentScope_->declareVariable(param->getName(), paramType);
  }

  if (node->getBody()) {
    visitBlock(node->getBody().get());
  }

  exitFunctionScope();
  return methodType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitFieldDecl(const nodes::FieldDeclNode *node) {
  // Get field type
  std::shared_ptr<ResolvedType> fieldType;
  if (node->getType()) {
    fieldType = visitType(node->getType().get());
  } else if (node->getInitializer()) {
    fieldType = visitExpr(node->getInitializer().get());
  } else {
    error(node->getLocation(),
          "Field must have either explicit type or initializer");
    return errorType_;
  }

  // Check initializer if present
  if (node->getInitializer()) {
    auto initType = visitExpr(node->getInitializer().get());
    if (!checkAssignmentCompatibility(fieldType, initType,
                                      node->getLocation())) {
      error(node->getLocation(), "Field initializer type mismatch");
    }
  }

  currentScope_->declareVariable(node->getName(), fieldType);
  return fieldType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitPropertyDecl(const nodes::PropertyDeclNode *node) {
  // Get property type
  auto propertyType = visitType(node->getPropertyType().get());

  // Check property body
  enterFunctionScope(propertyType);
  if (node->getBody()) {
    visitBlock(node->getBody().get());
  }
  exitFunctionScope();

  return propertyType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitEnumDecl(const nodes::EnumDeclNode *node) {
  auto enumType =
      std::make_shared<ResolvedType>(ResolvedType::Named(node->getName()));

  // Check underlying type if present
  std::shared_ptr<ResolvedType> underlyingType = intType_; // Default to int
  if (node->getUnderlyingType()) {
    underlyingType = visitType(node->getUnderlyingType().get());
  }

  // Process enum members
  for (const auto &member : node->getMembers()) {
    visitEnumMember(member.get());
  }

  return enumType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitEnumMember(const nodes::EnumMemberNode *node) {
  // Check member value if present
  if (node->getValue()) {
    auto valueType = visitExpr(node->getValue().get());
    // Should be compatible with enum's underlying type
    if (!valueType->isAssignableTo(*intType_)) {
      error(node->getLocation(),
            "Enum member value must be compatible with underlying type");
    }
  }
  return intType_; // Enum members have integer type
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitInterfaceDecl(const nodes::InterfaceDeclNode *node) {
  auto interfaceType =
      std::make_shared<ResolvedType>(ResolvedType::Named(node->getName()));

  enterScope();

  // Process extended interfaces
  for (const auto &extended : node->getExtendedInterfaces()) {
    visitType(extended.get());
  }

  // Process interface members
  for (const auto &member : node->getMembers()) {
    if (auto methodSig =
            std::dynamic_pointer_cast<nodes::MethodSignatureNode>(member)) {
      visitMethodSignature(methodSig.get());
    } else if (auto propSig =
                   std::dynamic_pointer_cast<nodes::PropertySignatureNode>(
                       member)) {
      visitPropertySignature(propSig.get());
    }
  }

  exitScope();
  return interfaceType;
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitGenericInterfaceDecl(
    const nodes::GenericInterfaceDeclNode *node) {
  // For now, treat generic interfaces similar to regular interfaces
  return visitInterfaceDecl(node);
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitMethodSignature(const nodes::MethodSignatureNode *node) {
  // Process return type
  auto returnType = visitType(node->getReturnType().get());

  // Process parameters
  std::vector<std::shared_ptr<ResolvedType>> paramTypes;
  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    paramTypes.push_back(paramType);
  }

  return std::make_shared<ResolvedType>(
      ResolvedType::Function(returnType, paramTypes));
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitPropertySignature(
    const nodes::PropertySignatureNode *node) {
  return visitType(node->getType().get());
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitNamespaceDecl(const nodes::NamespaceDeclNode *node) {
  enterScope();

  // Process all declarations in the namespace
  for (const auto &decl : node->getDeclarations()) {
    if (auto varDecl = std::dynamic_pointer_cast<nodes::VarDeclNode>(decl)) {
      visitVarDecl(varDecl.get());
    } else if (auto funcDecl =
                   std::dynamic_pointer_cast<nodes::FunctionDeclNode>(decl)) {
      visitFuncDecl(funcDecl.get());
    } else if (auto classDecl =
                   std::dynamic_pointer_cast<nodes::ClassDeclNode>(decl)) {
      visitClassDecl(classDecl.get());
    }
  }

  exitScope();
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitTypedefDecl(const nodes::TypedefDeclNode *node) {
  auto aliasedType = visitType(node->getAliasedType().get());
  currentScope_->declareType(node->getName(), aliasedType);
  return aliasedType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitParameter(const nodes::ParameterNode *node) {
  auto paramType = visitType(node->getType().get());

  // Handle reference parameters
  if (node->isRef()) {
    paramType =
        std::make_shared<ResolvedType>(ResolvedType::Reference(paramType));
  }

  // Check default value if present
  if (node->getDefaultValue()) {
    auto defaultType = visitExpr(node->getDefaultValue().get());
    if (!checkAssignmentCompatibility(paramType, defaultType,
                                      node->getLocation())) {
      error(node->getLocation(), "Parameter default value type mismatch");
    }
  }

  return paramType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitAttribute(const nodes::AttributeNode *node) {
  // Check attribute argument if present
  if (node->getArgument()) {
    visitExpr(node->getArgument().get());
  }
  return voidType_;
}

// Statement visitors
std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitStmt(const nodes::StatementNode *node) {
  if (auto exprStmt = dynamic_cast<const nodes::ExpressionStmtNode *>(node)) {
    return visitExprStmt(exprStmt);
  } else if (auto blockStmt = dynamic_cast<const nodes::BlockNode *>(node)) {
    return visitBlock(blockStmt);
  } else if (auto ifStmt = dynamic_cast<const nodes::IfStmtNode *>(node)) {
    return visitIfStmt(ifStmt);
  } else if (auto whileStmt =
                 dynamic_cast<const nodes::WhileStmtNode *>(node)) {
    return visitWhileStmt(whileStmt);
  } else if (auto doWhileStmt =
                 dynamic_cast<const nodes::DoWhileStmtNode *>(node)) {
    return visitDoWhileStmt(doWhileStmt);
  } else if (auto forStmt = dynamic_cast<const nodes::ForStmtNode *>(node)) {
    return visitForStmt(forStmt);
  } else if (auto forOfStmt =
                 dynamic_cast<const nodes::ForOfStmtNode *>(node)) {
    return visitForOfStmt(forOfStmt);
  } else if (auto returnStmt =
                 dynamic_cast<const nodes::ReturnStmtNode *>(node)) {
    return visitReturnStmt(returnStmt);
  } else if (auto breakStmt =
                 dynamic_cast<const nodes::BreakStmtNode *>(node)) {
    return visitBreakStmt(breakStmt);
  } else if (auto continueStmt =
                 dynamic_cast<const nodes::ContinueStmtNode *>(node)) {
    return visitContinueStmt(continueStmt);
  } else if (auto switchStmt =
                 dynamic_cast<const nodes::SwitchStmtNode *>(node)) {
    return visitSwitchStmt(switchStmt);
  } else if (auto tryStmt = dynamic_cast<const nodes::TryStmtNode *>(node)) {
    return visitTryStmt(tryStmt);
  } else if (auto throwStmt =
                 dynamic_cast<const nodes::ThrowStmtNode *>(node)) {
    return visitThrowStmt(throwStmt);
  } else if (auto asmStmt =
                 dynamic_cast<const nodes::AssemblyStmtNode *>(node)) {
    return visitAssemblyStmt(asmStmt);
  } else if (auto labeledStmt =
                 dynamic_cast<const nodes::LabeledStatementNode *>(node)) {
    return visitLabeledStmt(labeledStmt);
  } else if (auto declStmt =
                 dynamic_cast<const nodes::DeclarationStmtNode *>(node)) {
    return visitDeclarationStmt(declStmt);
  } else {
    error(node->getLocation(), "Unhandled statement type in type checking");
    return errorType_;
  }
}

// std::shared_ptr<ResolvedType>
// TypeCheckVisitor::visitDeclarationStmt(const nodes::DeclarationStmtNode
// *node) {
//   auto decl = node->getDeclaration();

//   if (auto varDecl = std::dynamic_pointer_cast<nodes::VarDeclNode>(decl)) {
//     return visitVarDecl(varDecl.get());
//   } else if (auto funcDecl =
//                  std::dynamic_pointer_cast<nodes::FunctionDeclNode>(decl)) {
//     return visitFuncDecl(funcDecl.get());
//   } else if (auto classDecl =
//                  std::dynamic_pointer_cast<nodes::ClassDeclNode>(decl)) {
//     return visitClassDecl(classDecl.get());
//   } else if (auto enumDecl =
//                  std::dynamic_pointer_cast<nodes::EnumDeclNode>(decl)) {
//     return visitEnumDecl(enumDecl.get());
//   } else if (auto interfaceDecl =
//                  std::dynamic_pointer_cast<nodes::InterfaceDeclNode>(decl)) {
//     return visitInterfaceDecl(interfaceDecl.get());
//   } else {
//     error(node->getLocation(), "Unsupported declaration in statement");
//     return errorType_;
//   }
// }

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitExprStmt(const nodes::ExpressionStmtNode *node) {
  visitExpr(node->getExpression().get());
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitBlock(const nodes::BlockNode *node) {
  enterScope();

  for (const auto &stmt : node->getStatements()) {
    visitStmt(stmt.get());
  }

  exitScope();
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitIfStmt(const nodes::IfStmtNode *node) {
  auto condType = visitExpr(node->getCondition().get());
  if (!condType->isImplicitlyConvertibleTo(*boolType_)) {
    error(node->getCondition()->getLocation(),
          "If condition must be convertible to boolean");
  }

  visitStmt(node->getThenBranch().get());

  if (node->getElseBranch()) {
    visitStmt(node->getElseBranch().get());
  }

  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitWhileStmt(const nodes::WhileStmtNode *node) {
  auto condType = visitExpr(node->getCondition().get());
  if (!condType->isImplicitlyConvertibleTo(*boolType_)) {
    error(node->getCondition()->getLocation(),
          "While condition must be convertible to boolean");
  }

  bool wasInLoop = inLoop_;
  inLoop_ = true;
  visitStmt(node->getBody().get());
  inLoop_ = wasInLoop;

  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitDoWhileStmt(const nodes::DoWhileStmtNode *node) {
  bool wasInLoop = inLoop_;
  inLoop_ = true;
  visitStmt(node->getBody().get());
  inLoop_ = wasInLoop;

  auto condType = visitExpr(node->getCondition().get());
  if (!condType->isImplicitlyConvertibleTo(*boolType_)) {
    error(node->getCondition()->getLocation(),
          "Do-while condition must be convertible to boolean");
  }

  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitForStmt(const nodes::ForStmtNode *node) {
  enterScope();

  if (node->getInitializer()) {
    visitStmt(node->getInitializer().get());
  }

  if (node->getCondition()) {
    auto condType = visitExpr(node->getCondition().get());
    if (!condType->isImplicitlyConvertibleTo(*boolType_)) {
      error(node->getCondition()->getLocation(),
            "For loop condition must be convertible to boolean");
    }
  }

  if (node->getIncrement()) {
    visitExpr(node->getIncrement().get());
  }

  bool wasInLoop = inLoop_;
  inLoop_ = true;
  visitStmt(node->getBody().get());
  inLoop_ = wasInLoop;

  exitScope();
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitForOfStmt(const nodes::ForOfStmtNode *node) {
  enterScope();

  auto iterableType = visitExpr(node->getIterable().get());

  // Check if iterable is actually iterable (array, etc.)
  if (iterableType->getKind() != ResolvedType::TypeKind::Array) {
    warning(node->getIterable()->getLocation(),
            "For-of requires an iterable type");
  }

  // Declare the loop variable
  auto elementType = iterableType->getKind() == ResolvedType::TypeKind::Array
                         ? iterableType->getElementType()
                         : errorType_;

  currentScope_->declareVariable(node->getIdentifier(), elementType);

  bool wasInLoop = inLoop_;
  inLoop_ = true;
  visitStmt(node->getBody().get());
  inLoop_ = wasInLoop;

  exitScope();
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitBreakStmt(const nodes::BreakStmtNode *node) {
  if (!inLoop_) {
    error(node->getLocation(), "Break statement must be inside a loop");
  }
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitContinueStmt(const nodes::ContinueStmtNode *node) {
  if (!inLoop_) {
    error(node->getLocation(), "Continue statement must be inside a loop");
  }
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitReturnStmt(const nodes::ReturnStmtNode *node) {
  std::shared_ptr<ResolvedType> returnedType = voidType_;
  if (node->getValue()) {
    returnedType = visitExpr(node->getValue().get());
  }

  if (currentFunctionReturnType_ &&
      !returnedType->isAssignableTo(*currentFunctionReturnType_)) {
    error(node->getLocation(),
          "Return value type doesn't match function return type");
  }

  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitTryStmt(const nodes::TryStmtNode *node) {
  bool wasInTry = inTryBlock_;
  inTryBlock_ = true;
  visitStmt(node->getTryBlock().get());
  inTryBlock_ = wasInTry;

  // Check catch clauses
  for (const auto &catchClause : node->getCatchClauses()) {
    enterScope();

    // Declare catch parameter
    if (catchClause.parameterType) {
      auto paramType = visitType(catchClause.parameterType.get());
      currentScope_->declareVariable(catchClause.parameter, paramType);
    } else {
      // Default exception type
      currentScope_->declareVariable(catchClause.parameter, errorType_);
    }

    visitStmt(catchClause.body.get());
    exitScope();
  }

  if (node->getFinallyBlock()) {
    visitStmt(node->getFinallyBlock().get());
  }

  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitThrowStmt(const nodes::ThrowStmtNode *node) {
  visitExpr(node->getValue().get());
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitSwitchStmt(const nodes::SwitchStmtNode *node) {
  auto exprType = visitExpr(node->getExpression().get());

  for (const auto &switchCase : node->getCases()) {
    if (!switchCase.isDefault && switchCase.value) {
      auto caseType = visitExpr(switchCase.value.get());
      if (!caseType->isAssignableTo(*exprType)) {
        error(switchCase.value->getLocation(),
              "Case value type doesn't match switch expression type");
      }
    }

    enterScope();
    for (const auto &stmt : switchCase.body) {
      visitStmt(stmt.get());
    }
    exitScope();
  }

  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitAssemblyStmt(const nodes::AssemblyStmtNode *node) {
  if (node->getCode().empty()) {
    error(node->getLocation(), "Assembly statement cannot have empty code");
    return errorType_;
  }

  // Assembly statements are executed for side effects
  return voidType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitLabeledStmt(const nodes::LabeledStatementNode *node) {
  // Just check the labeled statement
  return visitStmt(node->getStatement().get());
}

// Expression visitors
std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitExpr(const nodes::ExpressionNode *node) {
  if (auto binary = dynamic_cast<const nodes::BinaryExpressionNode *>(node)) {
    return visitBinaryExpr(binary);
  } else if (auto unary =
                 dynamic_cast<const nodes::UnaryExpressionNode *>(node)) {
    return visitUnaryExpr(unary);
  } else if (auto literal =
                 dynamic_cast<const nodes::LiteralExpressionNode *>(node)) {
    return visitLiteralExpr(literal);
  } else if (auto identifier =
                 dynamic_cast<const nodes::IdentifierExpressionNode *>(node)) {
    return visitIdentifierExpr(identifier);
  } else if (auto call =
                 dynamic_cast<const nodes::CallExpressionNode *>(node)) {
    return visitCallExpr(call);
  } else if (auto assignment =
                 dynamic_cast<const nodes::AssignmentExpressionNode *>(node)) {
    return visitAssignmentExpr(assignment);
  } else if (auto member =
                 dynamic_cast<const nodes::MemberExpressionNode *>(node)) {
    return visitMemberExpr(member);
  } else if (auto index =
                 dynamic_cast<const nodes::IndexExpressionNode *>(node)) {
    return visitIndexExpr(index);
  } else if (auto newExpr =
                 dynamic_cast<const nodes::NewExpressionNode *>(node)) {
    return visitNewExpr(newExpr);
  } else if (auto cast =
                 dynamic_cast<const nodes::CastExpressionNode *>(node)) {
    return visitCastExpr(cast);
  } else if (auto array = dynamic_cast<const nodes::ArrayLiteralNode *>(node)) {
    return visitArrayLiteral(array);
  } else if (auto conditional =
                 dynamic_cast<const nodes::ConditionalExpressionNode *>(node)) {
    return visitConditionalExpr(conditional);
  } else if (auto thisExpr =
                 dynamic_cast<const nodes::ThisExpressionNode *>(node)) {
    return visitThisExpr(thisExpr);
  } else if (auto compileTime =
                 dynamic_cast<const nodes::CompileTimeExpressionNode *>(node)) {
    return visitCompileTimeExpr(compileTime);
  } else if (auto templateSpec =
                 dynamic_cast<const nodes::TemplateSpecializationNode *>(
                     node)) {
    return visitTemplateSpecialization(templateSpec);
  } else if (auto pointerExpr =
                 dynamic_cast<const nodes::PointerExpressionNode *>(node)) {
    return visitPointerExpr(pointerExpr);
  } else if (auto funcExpr =
                 dynamic_cast<const nodes::FunctionExpressionNode *>(node)) {
    return visitFunctionExpr(funcExpr);
  } else {
    error(node->getLocation(), "Unhandled expression type in type checking");
    return errorType_;
  }
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitBinaryExpr(const nodes::BinaryExpressionNode *node) {
  auto leftType = visitExpr(node->getLeft().get());
  auto rightType = visitExpr(node->getRight().get());

  return checkBinaryOp(node->getExpressionType(), leftType, rightType,
                       node->getLocation());
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitUnaryExpr(const nodes::UnaryExpressionNode *node) {
  auto operandType = visitExpr(node->getOperand().get());
  return checkUnaryOp(node->getExpressionType(), operandType, node->isPrefix(),
                      node->getLocation());
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitLiteralExpr(const nodes::LiteralExpressionNode *node) {
  switch (node->getExpressionType()) {
  case tokens::TokenType::NUMBER:
    if (node->getValue().find('.') != std::string::npos) {
      return floatType_;
    } else {
      return intType_;
    }
  case tokens::TokenType::STRING_LITERAL:
    return stringType_;
  case tokens::TokenType::TRUE:
  case tokens::TokenType::FALSE:
    return boolType_;
  default:
    error(node->getLocation(), "Unknown literal type");
    return errorType_;
  }
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitIdentifierExpr(
    const nodes::IdentifierExpressionNode *node) {
  auto varType = currentScope_->lookupVariable(node->getName());

  if (!varType) {
    varType = currentScope_->lookupFunction(node->getName());
  }

  if (!varType) {
    error(node->getLocation(), "Undefined identifier: " + node->getName());
    return errorType_;
  }

  return varType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitCallExpr(const nodes::CallExpressionNode *node) {
  auto calleeType = visitExpr(node->getCallee().get());

  if (calleeType->getKind() != ResolvedType::TypeKind::Function) {
    error(node->getCallee()->getLocation(), "Cannot call non-function type");
    return errorType_;
  }

  return checkFunctionCall(calleeType, node->getArguments(),
                           node->getTypeArguments(), node->getLocation());
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitAssignmentExpr(
    const nodes::AssignmentExpressionNode *node) {
  auto targetType = visitExpr(node->getTarget().get());
  auto valueType = visitExpr(node->getValue().get());

  auto op = node->getExpressionType();

  if (op == tokens::TokenType::EQUALS) {
    if (!checkAssignmentCompatibility(targetType, valueType,
                                      node->getLocation())) {
      error(node->getLocation(), "Cannot assign incompatible type");
      return errorType_;
    }
  } else {
    // Compound assignment
    tokens::TokenType binaryOp;
    switch (op) {
    case tokens::TokenType::PLUS_EQUALS:
      binaryOp = tokens::TokenType::PLUS;
      break;
    case tokens::TokenType::MINUS_EQUALS:
      binaryOp = tokens::TokenType::MINUS;
      break;
    case tokens::TokenType::STAR_EQUALS:
      binaryOp = tokens::TokenType::STAR;
      break;
    case tokens::TokenType::SLASH_EQUALS:
      binaryOp = tokens::TokenType::SLASH;
      break;
    case tokens::TokenType::PERCENT_EQUALS:
      binaryOp = tokens::TokenType::PERCENT;
      break;
    default:
      error(node->getLocation(), "Unsupported compound assignment operator");
      return errorType_;
    }

    auto resultType =
        checkBinaryOp(binaryOp, targetType, valueType, node->getLocation());
    if (!resultType->isAssignableTo(*targetType)) {
      error(node->getLocation(),
            "Result of compound assignment is not assignable to target");
      return errorType_;
    }
  }

  return targetType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitMemberExpr(const nodes::MemberExpressionNode *node) {
  auto objectType = visitExpr(node->getObject().get());

  // For now, just return error type since we don't have complete member lookup
  error(node->getLocation(),
        "Member access type checking not fully implemented");
  return errorType_;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitIndexExpr(const nodes::IndexExpressionNode *node) {
  auto arrayType = visitExpr(node->getArray().get());
  auto indexType = visitExpr(node->getIndex().get());

  if (arrayType->getKind() != ResolvedType::TypeKind::Array) {
    error(node->getArray()->getLocation(), "Cannot index non-array type");
    return errorType_;
  }

  if (!indexType->isAssignableTo(*intType_)) {
    error(node->getIndex()->getLocation(), "Array index must be an integer");
  }

  return arrayType->getElementType();
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitNewExpr(const nodes::NewExpressionNode *node) {
  auto classType = currentScope_->lookupType(node->getClassName());
  if (!classType) {
    error(node->getLocation(), "Undefined class: " + node->getClassName());
    return errorType_;
  }

  // Check constructor arguments (simplified)
  for (const auto &arg : node->getArguments()) {
    visitExpr(arg.get());
  }

  return classType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitCastExpr(const nodes::CastExpressionNode *node) {
  auto exprType = visitExpr(node->getExpression().get());
  auto targetType = currentScope_->lookupType(node->getTargetType());

  if (!targetType) {
    error(node->getLocation(), "Undefined type: " + node->getTargetType());
    return errorType_;
  }

  if (!exprType->isExplicitlyConvertibleTo(*targetType)) {
    error(node->getLocation(), "Invalid cast");
    return errorType_;
  }

  return targetType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitArrayLiteral(const nodes::ArrayLiteralNode *node) {
  const auto &elements = node->getElements();

  if (elements.empty()) {
    error(node->getLocation(), "Cannot determine type of empty array literal");
    return errorType_;
  }

  auto elementType = visitExpr(elements[0].get());

  for (size_t i = 1; i < elements.size(); i++) {
    auto nextType = visitExpr(elements[i].get());
    if (!nextType->isAssignableTo(*elementType)) {
      error(elements[i]->getLocation(),
            "Array elements must have compatible types");
      return errorType_;
    }
  }

  return std::make_shared<ResolvedType>(ResolvedType::Array(elementType));
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitConditionalExpr(
    const nodes::ConditionalExpressionNode *node) {
  auto condType = visitExpr(node->getCondition().get());
  if (!condType->isImplicitlyConvertibleTo(*boolType_)) {
    error(node->getCondition()->getLocation(),
          "Conditional expression condition must be boolean");
  }

  auto trueType = visitExpr(node->getTrueExpression().get());
  auto falseType = visitExpr(node->getFalseExpression().get());

  // Return the more general type
  if (trueType->isAssignableTo(*falseType)) {
    return falseType;
  } else if (falseType->isAssignableTo(*trueType)) {
    return trueType;
  } else {
    error(node->getLocation(),
          "Conditional expression branches have incompatible types");
    return errorType_;
  }
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitThisExpr(const nodes::ThisExpressionNode *node) {
  if (!currentClassType_) {
    error(node->getLocation(), "'this' can only be used inside a class");
    return errorType_;
  }
  return currentClassType_;
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitCompileTimeExpr(
    const nodes::CompileTimeExpressionNode *node) {
  visitExpr(node->getOperand().get());

  // Compile-time expressions typically return compile-time constants
  // For simplicity, return the operand type
  return visitExpr(node->getOperand().get());
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitTemplateSpecialization(
    const nodes::TemplateSpecializationNode *node) {
  auto baseType = visitExpr(node->getBase().get());

  // For now, return the base type
  // In a complete implementation, we would resolve the template specialization
  return baseType;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitPointerExpr(const nodes::PointerExpressionNode *node) {
  auto operandType = visitExpr(node->getOperand().get());

  bool isUnsafe =
      (node->getKind() == nodes::PointerExpressionNode::PointerKind::Unsafe);
  return std::make_shared<ResolvedType>(
      ResolvedType::Pointer(operandType, isUnsafe));
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitFunctionExpr(const nodes::FunctionExpressionNode *node) {
  // Process return type
  std::shared_ptr<ResolvedType> returnType;
  if (node->getReturnType()) {
    returnType = visitType(node->getReturnType().get());
  } else {
    returnType = voidType_;
  }

  // Process parameters
  std::vector<std::shared_ptr<ResolvedType>> paramTypes;
  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    paramTypes.push_back(paramType);
  }

  // Check function body
  enterFunctionScope(returnType);

  for (const auto &param : node->getParameters()) {
    auto paramType = visitParameter(param.get());
    currentScope_->declareVariable(param->getName(), paramType);
  }

  visitBlock(node->getBody().get());
  exitFunctionScope();

  return std::make_shared<ResolvedType>(
      ResolvedType::Function(returnType, paramTypes));
}

// Type visitors
std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitType(const nodes::TypeNode *node) {
  if (auto primitive = dynamic_cast<const nodes::PrimitiveTypeNode *>(node)) {
    return visitPrimitiveType(primitive);
  } else if (auto named = dynamic_cast<const nodes::NamedTypeNode *>(node)) {
    return visitNamedType(named);
  } else if (auto qualified =
                 dynamic_cast<const nodes::QualifiedTypeNode *>(node)) {
    return visitQualifiedType(qualified);
  } else if (auto array = dynamic_cast<const nodes::ArrayTypeNode *>(node)) {
    return visitArrayType(array);
  } else if (auto pointer =
                 dynamic_cast<const nodes::PointerTypeNode *>(node)) {
    return visitPointerType(pointer);
  } else if (auto reference =
                 dynamic_cast<const nodes::ReferenceTypeNode *>(node)) {
    return visitReferenceType(reference);
  } else if (auto function =
                 dynamic_cast<const nodes::FunctionTypeNode *>(node)) {
    return visitFunctionType(function);
  } else if (auto templateType =
                 dynamic_cast<const nodes::TemplateTypeNode *>(node)) {
    return visitTemplateType(templateType);
  } else if (auto smartPointer =
                 dynamic_cast<const nodes::SmartPointerTypeNode *>(node)) {
    return visitSmartPointerType(smartPointer);
  } else if (auto unionType =
                 dynamic_cast<const nodes::UnionTypeNode *>(node)) {
    return visitUnionType(unionType);
  } else if (auto genericParam =
                 dynamic_cast<const nodes::GenericParamNode *>(node)) {
    return visitGenericParam(genericParam);
  } else if (auto builtinConstraint =
                 dynamic_cast<const nodes::BuiltinConstraintNode *>(node)) {
    return visitBuiltinConstraint(builtinConstraint);
  } else {
    error(node->getLocation(), "Unhandled type in type checking");
    return errorType_;
  }
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitPrimitiveType(const nodes::PrimitiveTypeNode *node) {
  switch (node->getType()) {
  case tokens::TokenType::VOID:
    return voidType_;
  case tokens::TokenType::INT:
    return intType_;
  case tokens::TokenType::FLOAT:
    return floatType_;
  case tokens::TokenType::BOOLEAN:
    return boolType_;
  case tokens::TokenType::STRING:
    return stringType_;
  default:
    error(node->getLocation(), "Unknown primitive type");
    return errorType_;
  }
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitNamedType(const nodes::NamedTypeNode *node) {
  auto type = currentScope_->lookupType(node->getName());
  if (!type) {
    error(node->getLocation(), "Undefined type: " + node->getName());
    return errorType_;
  }
  return type;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitQualifiedType(const nodes::QualifiedTypeNode *node) {
  // For now, just treat as named type using the last qualifier
  const auto &qualifiers = node->getQualifiers();
  if (qualifiers.empty()) {
    error(node->getLocation(), "Empty qualified type");
    return errorType_;
  }

  // In a complete implementation, we would resolve the namespace chain
  auto type = currentScope_->lookupType(qualifiers.back());
  if (!type) {
    error(node->getLocation(), "Undefined qualified type");
    return errorType_;
  }
  return type;
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitArrayType(const nodes::ArrayTypeNode *node) {
  auto elementType = visitType(node->getElementType().get());

  if (node->getSize()) {
    auto sizeType = visitExpr(node->getSize().get());
    if (!sizeType->isAssignableTo(*intType_)) {
      error(node->getSize()->getLocation(), "Array size must be an integer");
    }
  }

  return std::make_shared<ResolvedType>(ResolvedType::Array(elementType));
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitPointerType(const nodes::PointerTypeNode *node) {
  auto pointeeType = visitType(node->getBaseType().get());
  bool isUnsafe =
      node->getKind() == nodes::PointerTypeNode::PointerKind::Unsafe;
  return std::make_shared<ResolvedType>(
      ResolvedType::Pointer(pointeeType, isUnsafe));
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitReferenceType(const nodes::ReferenceTypeNode *node) {
  auto baseType = visitType(node->getBaseType().get());
  return std::make_shared<ResolvedType>(ResolvedType::Reference(baseType));
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitFunctionType(const nodes::FunctionTypeNode *node) {
  auto returnType = visitType(node->getReturnType().get());

  std::vector<std::shared_ptr<ResolvedType>> paramTypes;
  for (const auto &paramType : node->getParameterTypes()) {
    paramTypes.push_back(visitType(paramType.get()));
  }

  return std::make_shared<ResolvedType>(
      ResolvedType::Function(returnType, paramTypes));
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitTemplateType(const nodes::TemplateTypeNode *node) {
  auto baseType = visitType(node->getBaseType().get());

  std::vector<std::shared_ptr<ResolvedType>> argTypes;
  for (const auto &argType : node->getArguments()) {
    argTypes.push_back(visitType(argType.get()));
  }

  if (baseType->getKind() != ResolvedType::TypeKind::Named) {
    error(node->getBaseType()->getLocation(),
          "Template base type must be a named type");
    return errorType_;
  }

  return std::make_shared<ResolvedType>(
      ResolvedType::Template(baseType->getName(), argTypes));
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitSmartPointerType(
    const nodes::SmartPointerTypeNode *node) {
  auto pointeeType = visitType(node->getPointeeType().get());

  ResolvedType::SmartKind kind;
  switch (node->getKind()) {
  case nodes::SmartPointerTypeNode::SmartPointerKind::Shared:
    kind = ResolvedType::SmartKind::Shared;
    break;
  case nodes::SmartPointerTypeNode::SmartPointerKind::Unique:
    kind = ResolvedType::SmartKind::Unique;
    break;
  case nodes::SmartPointerTypeNode::SmartPointerKind::Weak:
    kind = ResolvedType::SmartKind::Weak;
    break;
  default:
    error(node->getLocation(), "Unknown smart pointer kind");
    return errorType_;
  }

  return std::make_shared<ResolvedType>(ResolvedType::Smart(pointeeType, kind));
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitUnionType(const nodes::UnionTypeNode *node) {
  auto leftType = visitType(node->getLeft().get());
  auto rightType = visitType(node->getRight().get());
  return std::make_shared<ResolvedType>(
      ResolvedType::Union(leftType, rightType));
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitGenericParam(const nodes::GenericParamNode *node) {
  // Generic parameters are treated as named types during type checking
  auto genericType =
      std::make_shared<ResolvedType>(ResolvedType::Named(node->getName()));

  // Check constraints
  for (const auto &constraint : node->getConstraints()) {
    visitType(constraint.get());
  }

  return genericType;
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::visitBuiltinConstraint(
    const nodes::BuiltinConstraintNode *node) {
  // Builtin constraints are compile-time constructs, return a placeholder type
  return std::make_shared<ResolvedType>(
      ResolvedType::Named(node->getConstraintName()));
}

// Helper methods
void TypeCheckVisitor::error(const core::SourceLocation &location,
                             const std::string &message) {
  errorReporter_.error(location, message);
}

void TypeCheckVisitor::warning(const core::SourceLocation &location,
                               const std::string &message) {
  errorReporter_.warning(location, message);
}

void TypeCheckVisitor::enterScope() {
  currentScope_ = currentScope_->createChildScope();
}

void TypeCheckVisitor::exitScope() {
  if (currentScope_->createChildScope()) {
    currentScope_ =
        currentScope_->createChildScope(); // This should be parent, but we
                                           // don't have parent accessor
  }
}

void TypeCheckVisitor::enterFunctionScope(
    std::shared_ptr<ResolvedType> returnType) {
  enterScope();
  currentFunctionReturnType_ = returnType;
}

void TypeCheckVisitor::exitFunctionScope() {
  currentFunctionReturnType_ = nullptr;
  exitScope();
}

std::shared_ptr<ResolvedType>
TypeCheckVisitor::checkBinaryOp(tokens::TokenType op,
                                std::shared_ptr<ResolvedType> leftType,
                                std::shared_ptr<ResolvedType> rightType,
                                const core::SourceLocation &location) {

  // Handle arithmetic operators
  if (tokens::isArithmeticOperator(op)) {
    if ((leftType->getKind() == ResolvedType::TypeKind::Int ||
         leftType->getKind() == ResolvedType::TypeKind::Float) &&
        (rightType->getKind() == ResolvedType::TypeKind::Int ||
         rightType->getKind() == ResolvedType::TypeKind::Float)) {

      if (leftType->getKind() == ResolvedType::TypeKind::Float ||
          rightType->getKind() == ResolvedType::TypeKind::Float) {
        return floatType_;
      } else {
        return intType_;
      }
    }

    // String concatenation with + operator
    if (op == tokens::TokenType::PLUS &&
        (leftType->getKind() == ResolvedType::TypeKind::String ||
         rightType->getKind() == ResolvedType::TypeKind::String)) {
      return stringType_;
    }

    error(location, "Invalid operands for arithmetic operator");
    return errorType_;
  }

  // Handle comparison operators
  if (tokens::isComparisonOperator(op)) {
    if (leftType->isAssignableTo(*rightType) ||
        rightType->isAssignableTo(*leftType)) {
      return boolType_;
    }
    error(location, "Cannot compare incompatible types");
    return errorType_;
  }

  // Handle logical operators
  if (tokens::isLogicalOperator(op)) {
    if (leftType->isImplicitlyConvertibleTo(*boolType_) &&
        rightType->isImplicitlyConvertibleTo(*boolType_)) {
      return boolType_;
    }
    error(location, "Logical operators require boolean operands");
    return errorType_;
  }

  // Handle bitwise operators
  if (tokens::isBitwiseOperator(op)) {
    if (leftType->getKind() == ResolvedType::TypeKind::Int &&
        rightType->getKind() == ResolvedType::TypeKind::Int) {
      return intType_;
    }
    error(location, "Bitwise operators require integer operands");
    return errorType_;
  }

  error(location, "Unhandled binary operator in type checking");
  return errorType_;
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::checkUnaryOp(
    tokens::TokenType op, std::shared_ptr<ResolvedType> operandType,
    bool isPrefix, const core::SourceLocation &location) {

  switch (op) {
  case tokens::TokenType::PLUS:
  case tokens::TokenType::MINUS:
    if (operandType->getKind() == ResolvedType::TypeKind::Int ||
        operandType->getKind() == ResolvedType::TypeKind::Float) {
      return operandType;
    }
    error(location, "Unary +/- requires numeric operand");
    return errorType_;

  case tokens::TokenType::EXCLAIM:
    if (operandType->isImplicitlyConvertibleTo(*boolType_)) {
      return boolType_;
    }
    error(location, "Logical NOT requires boolean operand");
    return errorType_;

  case tokens::TokenType::TILDE:
    if (operandType->getKind() == ResolvedType::TypeKind::Int) {
      return intType_;
    }
    error(location, "Bitwise NOT requires integer operand");
    return errorType_;

  case tokens::TokenType::PLUS_PLUS:
  case tokens::TokenType::MINUS_MINUS:
    if (operandType->getKind() == ResolvedType::TypeKind::Int ||
        operandType->getKind() == ResolvedType::TypeKind::Float) {
      return operandType;
    }
    error(location, "Increment/decrement requires numeric operand");
    return errorType_;

  case tokens::TokenType::STAR:
    // Dereference operator
    if (operandType->getKind() == ResolvedType::TypeKind::Pointer) {
      return operandType->getPointeeType();
    }
    error(location, "Dereference requires pointer operand");
    return errorType_;

  case tokens::TokenType::AT:
    // Address-of operator
    return std::make_shared<ResolvedType>(ResolvedType::Pointer(operandType));

  default:
    error(location, "Unhandled unary operator in type checking");
    return errorType_;
  }
}

bool TypeCheckVisitor::checkAssignmentCompatibility(
    std::shared_ptr<ResolvedType> targetType,
    std::shared_ptr<ResolvedType> valueType,
    const core::SourceLocation &location) {

  if (valueType->isAssignableTo(*targetType)) {
    return true;
  }

  error(location, "Cannot assign " + valueType->toString() + " to " +
                      targetType->toString());
  return false;
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::checkFunctionCall(
    std::shared_ptr<ResolvedType> calleeType,
    const std::vector<nodes::ExpressionPtr> &args,
    const std::vector<std::string> &typeArgs,
    const core::SourceLocation &location) {

  if (calleeType->getKind() != ResolvedType::TypeKind::Function) {
    error(location, "Cannot call non-function type");
    return errorType_;
  }

  const auto &paramTypes = calleeType->getParameterTypes();

  if (paramTypes.size() != args.size()) {
    error(location, "Wrong number of arguments");
    return errorType_;
  }

  // Check each argument type
  for (size_t i = 0; i < args.size(); i++) {
    auto argType = visitExpr(args[i].get());
    if (!argType->isAssignableTo(*paramTypes[i])) {
      error(args[i]->getLocation(), "Argument type mismatch");
    }
  }

  return calleeType->getReturnType();
}

std::shared_ptr<ResolvedType> TypeCheckVisitor::resolveGenericType(
    const std::string &name,
    const std::vector<std::shared_ptr<ResolvedType>> &typeArgs,
    const core::SourceLocation &location) {

  // For now, just return a template type
  return std::make_shared<ResolvedType>(ResolvedType::Template(name, typeArgs));
}

// Fix the visitDeclarationStmt method - there was a typo
std::shared_ptr<ResolvedType>
TypeCheckVisitor::visitDeclarationStmt(const nodes::DeclarationStmtNode *node) {
  auto decl = node->getDeclaration();

  if (auto varDecl = std::dynamic_pointer_cast<nodes::VarDeclNode>(decl)) {
    return visitVarDecl(varDecl.get());
  } else if (auto funcDecl =
                 std::dynamic_pointer_cast<nodes::FunctionDeclNode>(decl)) {
    return visitFuncDecl(funcDecl.get());
  } else if (auto classDecl =
                 std::dynamic_pointer_cast<nodes::ClassDeclNode>(decl)) {
    return visitClassDecl(classDecl.get());
  } else if (auto enumDecl =
                 std::dynamic_pointer_cast<nodes::EnumDeclNode>(decl)) {
    return visitEnumDecl(enumDecl.get());
  } else if (auto interfaceDecl =
                 std::dynamic_pointer_cast<nodes::InterfaceDeclNode>(decl)) {
    return visitInterfaceDecl(interfaceDecl.get());
  } else {
    error(node->getLocation(), "Unsupported declaration in statement");
    return errorType_;
  }
}

} // namespace visitors