#pragma once
#include "core/diagnostics/error_reporter.h"
#include "ideclaration_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"

namespace visitors {

class ExpressionParseVisitor;
class DeclarationParseVisitor;

class VariableDeclarationVisitor {
public:
  VariableDeclarationVisitor(tokens::TokenStream &tokens,
                             core::ErrorReporter &errorReporter,
                             IExpressionVisitor &exprVisitor,
                             IDeclarationVisitor &declVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        exprVisitor_(exprVisitor), declVisitor_(declVisitor) {}

  nodes::DeclPtr parseVarDecl(bool isConst, tokens::TokenType storageClass) {
    auto location = tokens_.peek().getLocation();

    if (!match(tokens::TokenType::IDENTIFIER)) {
      error("Expected variable name");
      return nullptr;
    }
    auto name = tokens_.previous().getLexeme();

    // Parse type annotation if present
    nodes::TypePtr type;
    if (match(tokens::TokenType::COLON)) {
      type = declVisitor_.parseType();
      if (!type)
        return nullptr;
    }

    // Parse initializer if present
    nodes::ExpressionPtr initializer;
    if (match(tokens::TokenType::EQUALS)) {
      initializer = exprVisitor_.parseExpression();
      if (!initializer)
        return nullptr;
    } else if (isConst) {
      error("Const declarations must have an initializer");
      return nullptr;
    }

    if (!consume(tokens::TokenType::SEMICOLON,
                 "Expected ';' after variable declaration")) {
      return nullptr;
    }

    // Create variable declaration with storage class
    return std::make_shared<nodes::VarDeclNode>(
        name, type, initializer,
        storageClass, // Pass through the storage class
        isConst, location);
  }

private:
  inline bool match(tokens::TokenType type) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    return false;
  }

  inline bool check(tokens::TokenType type) const {
    return !tokens_.isAtEnd() && tokens_.peek().getType() == type;
  }

  inline bool consume(tokens::TokenType type, const std::string &message) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    error(message);
    return false;
  }

  inline void error(const std::string &message) {
    errorReporter_.error(tokens_.peek().getLocation(), message);
  }

  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &exprVisitor_;
  IDeclarationVisitor &declVisitor_;
};
} // namespace visitors