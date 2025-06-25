#pragma once
#include "parser/nodes/declaration_nodes.h"
#include "parser/nodes/type_nodes.h"

// Add to ideclaration_visitor.h
class IDeclarationVisitor {
public:
  virtual ~IDeclarationVisitor() = default;
  virtual nodes::TypePtr parseType() = 0;
  virtual nodes::BlockPtr parseBlock() = 0;
  virtual nodes::DeclPtr parseDeclaration() = 0;

  // Add these new methods
  virtual nodes::DeclPtr parseNamespaceDecl() { return parseDeclaration(); }
  virtual nodes::DeclPtr parseInterfaceDecl() { return parseDeclaration(); }
  virtual nodes::DeclPtr parseEnumDecl() { return parseDeclaration(); }
  virtual nodes::DeclPtr parseTypedefDecl() { return parseDeclaration(); }
};