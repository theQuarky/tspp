#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/expression_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"
#include "tokens/token_type.h"

namespace visitors {

class ExpressionParseVisitor;

class UnaryExpressionVisitor {
public:
  UnaryExpressionVisitor(tokens::TokenStream &tokens,
                         core::ErrorReporter &errorReporter,
                         IExpressionVisitor &parent)
      : tokens_(tokens), errorReporter_(errorReporter), parentVisitor_(parent) {
  }

  nodes::ExpressionPtr parseUnary() {
    // Check for NEW keyword first (high precedence)
    if (tokens_.peek().getType() == tokens::TokenType::NEW) {
      tokens_.advance();
      // Use the parent's parseNewExpression method
      return parentVisitor_.parseNewExpression();
    }

    // Check for prefix operators next
    if (isUnaryPrefixOperator(tokens_.peek().getType())) {
      auto op = tokens_.advance();
      auto operand = parseUnary();
      if (!operand)
        return nullptr;
      return std::make_shared<nodes::UnaryExpressionNode>(
          op.getLocation(), op.getType(), operand, true // isPrefix = true
      );
    }

    // Parse primary expression
    auto expr = parentVisitor_.parsePrimary();
    if (!expr)
      return nullptr;

    // Check for postfix operators (++, --)
    while (isUnaryPostfixOperator(tokens_.peek().getType())) {
      auto op = tokens_.advance();
      expr = std::make_shared<nodes::UnaryExpressionNode>(
          op.getLocation(), op.getType(), expr, false // isPrefix = false
      );
    }

    return expr;
  }

private:
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &parentVisitor_;

  bool isUnaryPrefixOperator(tokens::TokenType type) const {
    switch (type) {
    case tokens::TokenType::MINUS:       // -
    case tokens::TokenType::EXCLAIM:     // !
    case tokens::TokenType::TILDE:       // ~
    case tokens::TokenType::PLUS_PLUS:   // ++
    case tokens::TokenType::MINUS_MINUS: // --
    case tokens::TokenType::STAR:        // *
    case tokens::TokenType::AT:
      return true;
    default:
      return false;
    }
  }

  bool isUnaryPostfixOperator(tokens::TokenType type) const {
    switch (type) {
    case tokens::TokenType::PLUS_PLUS:   // ++
    case tokens::TokenType::MINUS_MINUS: // --
      return true;
    default:
      return false;
    }
  }

  bool isUnaryOperator(tokens::TokenType type) const {
    switch (type) {
    case tokens::TokenType::MINUS:       // -
    case tokens::TokenType::EXCLAIM:     // !
    case tokens::TokenType::TILDE:       // ~
    case tokens::TokenType::PLUS_PLUS:   // ++
    case tokens::TokenType::MINUS_MINUS: // --
      return true;
    default:
      return false;
    }
  }
};
} // namespace visitors