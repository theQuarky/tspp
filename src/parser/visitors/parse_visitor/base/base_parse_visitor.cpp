// base_parse_visitor.cpp
#include "base_parse_visitor.h"
#include "tokens/token_type.h"
#include <iostream>

namespace visitors {

BaseParseVisitor::BaseParseVisitor(tokens::TokenStream &tokens,
                                   core::ErrorReporter &errorReporter)
    : tokens_(tokens), errorReporter_(errorReporter) {
  // Initialize in the correct order
  expressionVisitor_ =
      std::make_unique<ExpressionParseVisitor>(tokens, errorReporter);

  // Create statement visitor with expression visitor
  statementVisitor_ = std::make_unique<StatementParseVisitor>(
      tokens, errorReporter, *expressionVisitor_);

  // Create declaration visitor
  declarationVisitor_ = std::make_unique<DeclarationParseVisitor>(
      tokens, errorReporter, *expressionVisitor_, *statementVisitor_);

  // Now set the declaration visitor on the statement visitor
  statementVisitor_->setDeclarationVisitor(declarationVisitor_.get());

  // Also set the declaration visitor on the expression visitor
  expressionVisitor_->setDeclarationVisitor(declarationVisitor_.get());

  // And set the statement visitor on the expression visitor
  expressionVisitor_->setStatementVisitor(statementVisitor_.get());
}

// Rest of the implementation remains the same
bool BaseParseVisitor::visitParse() {
  try {
    bool hadError = false;

    while (!tokens_.isAtEnd()) {
      if (isDeclarationStart()) {
        auto declNode = parseDeclaration();
        if (!declNode) {
          hadError = true;
          synchronize();
          continue;
        }
        nodes_.push_back(std::move(declNode));
      } else {
        auto stmtNode = parseStatement();
        if (!stmtNode) {
          hadError = true;
          synchronize();
          continue;
        }
        nodes_.push_back(std::move(stmtNode));
      }
    }

    return !hadError;
  } catch (const std::exception &e) {
    error(std::string("Error during parsing: ") + e.what());
    return false;
  }
}

nodes::NodePtr BaseParseVisitor::parseDeclaration() {
  return declarationVisitor_->parseDeclaration();
}

nodes::NodePtr BaseParseVisitor::parseStatement() {
  return statementVisitor_->parseStatement();
}

bool BaseParseVisitor::isDeclarationStart() const {
  return tokens_.check(tokens::TokenType::STACK) ||
         tokens_.check(tokens::TokenType::HEAP) ||
         tokens_.check(tokens::TokenType::STATIC) ||
         tokens_.check(tokens::TokenType::PACKED) ||
         tokens_.check(tokens::TokenType::ALIGNED) ||
         tokens_.check(tokens::TokenType::ABSTRACT) ||
         tokens_.check(tokens::TokenType::ATTRIBUTE) ||
         tokens_.check(tokens::TokenType::LET) ||
         tokens_.check(tokens::TokenType::CONST) ||
         (tokens_.check(tokens::TokenType::FUNCTION) &&
          tokens_.peek().getType() == tokens::TokenType::IDENTIFIER) ||
         tokens_.check(tokens::TokenType::CLASS) ||
         tokens_.check(tokens::TokenType::PUBLIC) ||
         tokens_.check(tokens::TokenType::PRIVATE) ||
         tokens_.check(tokens::TokenType::PROTECTED) ||
         tokens_.check(tokens::TokenType::INTERFACE) ||
         tokens_.check(tokens::TokenType::ENUM) ||
         tokens_.check(tokens::TokenType::NAMESPACE) ||
         tokens_.check(tokens::TokenType::TYPEDEF) ||
         tokens_.check(tokens::TokenType::ZEROCAST) ||
         tokens::isFunctionModifier(tokens_.peek().getType());
}

void BaseParseVisitor::synchronize() {
  tokens_.advance();

  while (!tokens_.isAtEnd()) {
    if (tokens_.previous().getType() == tokens::TokenType::SEMICOLON) {
      return;
    }

    switch (tokens_.peek().getType()) {
    case tokens::TokenType::CLASS:
    case tokens::TokenType::FUNCTION:
    case tokens::TokenType::LET:
    case tokens::TokenType::CONST:
    case tokens::TokenType::IF:
    case tokens::TokenType::WHILE:
    case tokens::TokenType::RETURN:
      return;
    default:
      tokens_.advance();
    }
  }
}

void BaseParseVisitor::error(const std::string &message) {
  errorReporter_.error(tokens_.peek().getLocation(), message);
}

} // namespace visitors