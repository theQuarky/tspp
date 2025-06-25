#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/expression_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"

namespace visitors {

class ExpressionParseVisitor; // Forward declaration

class BinaryExpressionVisitor {
public:
  BinaryExpressionVisitor(tokens::TokenStream &tokens,
                          core::ErrorReporter &errorReporter,
                          IExpressionVisitor &parent)
      : tokens_(tokens), errorReporter_(errorReporter), parentVisitor_(parent) {
  }

  nodes::ExpressionPtr parseBinary(nodes::ExpressionPtr left,
                                   int minPrecedence) {
    while (true) {
      auto token = tokens_.peek();

      if (!isBinaryOperator(token.getType()) ||
          getOperatorPrecedence(token.getType()) < minPrecedence) {
        break;
      }

      tokens_.advance();

      // Direct call to parent visitor instead of using callback
      auto right = parentVisitor_.parseUnary();
      if (!right)
        return nullptr;

      // Create binary expression node
      left = std::make_shared<nodes::BinaryExpressionNode>(
          token.getLocation(), token.getType(), left, right);
    }
    return left;
  }

private:
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &parentVisitor_; // Changed to interface reference

  int getOperatorPrecedence(tokens::TokenType type) const {
    switch (type) {
    case tokens::TokenType::STAR:
    case tokens::TokenType::SLASH:
    case tokens::TokenType::PERCENT:
      return 13; // Multiplicative

    case tokens::TokenType::PLUS:
    case tokens::TokenType::MINUS:
      return 12; // Additive

    case tokens::TokenType::LEFT_SHIFT:
    case tokens::TokenType::RIGHT_SHIFT:
      return 11; // Shift

    case tokens::TokenType::LESS:
    case tokens::TokenType::GREATER:
    case tokens::TokenType::LESS_EQUALS:
    case tokens::TokenType::GREATER_EQUALS:
      return 10; // Relational

    case tokens::TokenType::EQUALS_EQUALS:
    case tokens::TokenType::EXCLAIM_EQUALS:
      return 9; // Equality

    case tokens::TokenType::AMPERSAND:
      return 8; // Bitwise AND

    case tokens::TokenType::CARET:
      return 7; // Bitwise XOR

    case tokens::TokenType::PIPE:
      return 6; // Bitwise OR

    case tokens::TokenType::AMPERSAND_AMPERSAND:
      return 5; // Logical AND

    case tokens::TokenType::PIPE_PIPE:
      return 4; // Logical OR

    default:
      return -1; // Not a binary operator
    }
  }

  bool isBinaryOperator(tokens::TokenType type) const {
    return getOperatorPrecedence(type) >= 0;
  }
};

} // namespace visitors