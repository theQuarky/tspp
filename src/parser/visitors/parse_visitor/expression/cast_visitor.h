#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/expression_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"

namespace visitors {

class ExpressionParseVisitor;

class CastExpressionVisitor {
public:
  CastExpressionVisitor(tokens::TokenStream &tokens,
                        core::ErrorReporter &errorReporter,
                        IExpressionVisitor &parent)
      : tokens_(tokens), errorReporter_(errorReporter), parentVisitor_(parent) {
  }

  nodes::ExpressionPtr parseCast() {
    auto location = tokens_.peek().getLocation();

    // Skip 'cast'
    tokens_.advance();

    if (!match(tokens::TokenType::LESS)) {
      error("Expected '<' after 'cast'");
      return nullptr;
    }

    // Parse the target type
    // Note: For now, we'll just collect the type name as a string
    // This should be replaced with proper type parsing once implemented
    if (!match(tokens::TokenType::IDENTIFIER)) {
      error("Expected type name in cast expression");
      return nullptr;
    }
    std::string typeName = tokens_.previous().getLexeme();

    if (!match(tokens::TokenType::GREATER)) {
      error("Expected '>' after type in cast expression");
      return nullptr;
    }

    // Parse the expression to be cast
    auto expression = parentVisitor_.parseExpression();
    if (!expression)
      return nullptr;

    return std::make_shared<nodes::CastExpressionNode>(location, typeName,
                                                       expression);
  }

private:
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &parentVisitor_;

  bool match(tokens::TokenType type) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    return false;
  }

  bool check(tokens::TokenType type) const {
    return !tokens_.isAtEnd() && tokens_.peek().getType() == type;
  }

  void error(const std::string &message) {
    errorReporter_.error(tokens_.peek().getLocation(), message);
  }
};
} // namespace visitors