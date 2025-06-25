// call_visitor.h
#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/expression_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"

namespace visitors {

class ExpressionParseVisitor;

class CallExpressionVisitor {
public:
  CallExpressionVisitor(tokens::TokenStream &tokens,
                        core::ErrorReporter &errorReporter,
                        IExpressionVisitor &parent)
      : tokens_(tokens), errorReporter_(errorReporter), parentVisitor_(parent) {
  }

  nodes::ExpressionPtr parseCallOrMember(nodes::ExpressionPtr expr) {
    while (true) {
      if (match(tokens::TokenType::LEFT_PAREN)) {
        expr = finishCall(expr);
        if (!expr)
          return nullptr;
      } else if (match(tokens::TokenType::DOT)) {
        if (!match(tokens::TokenType::IDENTIFIER)) {
          error("Expected property name after '.'");
          return nullptr;
        }
        expr = std::make_shared<nodes::MemberExpressionNode>(
            tokens_.previous().getLocation(), expr,
            tokens_.previous().getLexeme(),
            false // not a pointer access
        );
      } else if (match(tokens::TokenType::AT)) {
        if (!match(tokens::TokenType::IDENTIFIER)) {
          error("Expected property name after '@'");
          return nullptr;
        }
        expr = std::make_shared<nodes::MemberExpressionNode>(
            tokens_.previous().getLocation(), expr,
            tokens_.previous().getLexeme(),
            true // pointer access
        );
      } else if (match(tokens::TokenType::LEFT_BRACKET)) {
        // Handle index access
        auto index = parentVisitor_.parseExpression();
        if (!index)
          return nullptr;

        if (!consume(tokens::TokenType::RIGHT_BRACKET,
                     "Expected ']' after index")) {
          return nullptr;
        }

        expr = std::make_shared<nodes::IndexExpressionNode>(
            tokens_.previous().getLocation(), expr, index);
      } else {
        break;
      }
    }
    return expr;
  }

private:
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &parentVisitor_; // Changed to interface reference

  nodes::ExpressionPtr finishCall(nodes::ExpressionPtr callee) {
    std::vector<nodes::ExpressionPtr> arguments;

    // Handle empty argument list
    if (!check(tokens::TokenType::RIGHT_PAREN)) {
      do {
        if (arguments.size() >= 255) {
          error("Cannot have more than 255 arguments");
          return nullptr;
        }

        auto arg = parentVisitor_.parseExpression();
        if (!arg)
          return nullptr;
        arguments.push_back(std::move(arg));
      } while (match(tokens::TokenType::COMMA));
    }

    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after arguments")) {
      return nullptr;
    }

    return std::make_shared<nodes::CallExpressionNode>(
        callee->getLocation(), callee, std::move(arguments));
  }

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

  bool consume(tokens::TokenType type, const std::string &message) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    error(message);
    return false;
  }

  void error(const std::string &message) {
    errorReporter_.error(tokens_.peek().getLocation(), message);
  }
};

} // namespace visitors
