#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/expression_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"
#include "tokens/token_type.h"
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace visitors {

class ExpressionParseVisitor;

class PrimaryExpressionVisitor {
public:
  PrimaryExpressionVisitor(tokens::TokenStream &tokens,
                           core::ErrorReporter &errorReporter,
                           IExpressionVisitor &parent)
      : tokens_(tokens), errorReporter_(errorReporter), parentVisitor_(parent) {
  }

  // Parses a primary expression (array literals, "this", identifiers, literals,
  // or parenthesized expressions)
  // Parses a primary expression (array literals, "this", identifiers, literals,
  // or parenthesized expressions)
  nodes::ExpressionPtr parsePrimary() {
    // Handle array literals: [1, 2, 3]
    if (match(tokens::TokenType::LEFT_BRACKET)) {
      return parseArrayLiteral();
    }

    // Handle the "this" keyword
    if (match(tokens::TokenType::THIS)) {
      auto token = tokens_.previous();
      auto expr =
          std::make_shared<nodes::ThisExpressionNode>(token.getLocation());
      return parsePostfixOperations(expr);
    }

    // Handle identifiers
    if (match(tokens::TokenType::IDENTIFIER)) {
      auto token = tokens_.previous();
      auto expr = std::make_shared<nodes::IdentifierExpressionNode>(
          token.getLocation(), token.getLexeme());

      // Check for generic function call with lookahead
      if (check(tokens::TokenType::LESS)) {
        // Save current position before lookahead
        size_t savedPos = tokens_.savePosition();

        tokens_.advance(); // Consume '<'

        // Check if next token is a type name (identifier or primitive type)
        bool isGenericCall = false;
        if (check(tokens::TokenType::IDENTIFIER) ||
            (tokens_.peek().getType() >= tokens::TokenType::TYPE_BEGIN &&
             tokens_.peek().getType() <= tokens::TokenType::TYPE_END)) {

          tokens_.advance(); // Skip type name

          // Keep going until we find '>' or fail
          while (!tokens_.isAtEnd()) {
            // If we find a '>', it's a valid generic syntax
            if (check(tokens::TokenType::GREATER)) {
              // Look ahead one more to see if followed by '('
              tokens_.advance(); // Consume '>'
              if (check(tokens::TokenType::LEFT_PAREN)) {
                isGenericCall = true;
              }
              break;
            }
            // Allow commas for multiple type arguments
            else if (check(tokens::TokenType::COMMA)) {
              tokens_.advance();
              // Next token should be a type
              if (!(check(tokens::TokenType::IDENTIFIER) ||
                    (tokens_.peek().getType() >=
                         tokens::TokenType::TYPE_BEGIN &&
                     tokens_.peek().getType() <=
                         tokens::TokenType::TYPE_END))) {
                break; // Not a valid generic syntax
              }
              tokens_.advance(); // Skip type name
            } else {
              // Not a valid generic syntax
              break;
            }
          }
        }

        // Restore position regardless of outcome
        tokens_.restorePosition(savedPos);

        // If we detected a generic call, parse it
        if (isGenericCall) {
          return parseGenericFunctionCall(expr);
        }
      }

      // Normal postfix operations (not a generic call)
      return parsePostfixOperations(expr);
    }

    // Handle literals (numbers, strings, booleans)
    if (match(tokens::TokenType::NUMBER) ||
        match(tokens::TokenType::STRING_LITERAL) ||
        match(tokens::TokenType::TRUE) || match(tokens::TokenType::FALSE)) {
      auto token = tokens_.previous();
      auto expr = std::make_shared<nodes::LiteralExpressionNode>(
          token.getLocation(), token.getType(), token.getLexeme());
      return parsePostfixOperations(expr);
    }

    // Handle parenthesized expressions: ( expression )
    if (match(tokens::TokenType::LEFT_PAREN)) {
      auto expr = parentVisitor_.parseExpression();
      if (!expr)
        return nullptr;
      if (!consume(tokens::TokenType::RIGHT_PAREN,
                   "Expected ')' after expression")) {
        return nullptr;
      }
      return parsePostfixOperations(expr);
    }

    if(check(tokens::TokenType::FUNCTION)){
      return parentVisitor_.parseFunctionExpression();
    }

    error("Expected expression");
    return nullptr;
  }

  // Parse a generic function call: identifier<Type>(args)
  nodes::ExpressionPtr parseGenericFunctionCall(nodes::ExpressionPtr expr) {
    // Parse the opening "<" - we already checked it exists
    tokens_.advance(); // Consume '<'

    // Parse type arguments
    std::vector<std::string> typeArgs;
    do {
      // Check for a type name (identifier or primitive type)
      if (check(tokens::TokenType::IDENTIFIER) ||
          (tokens_.peek().getType() >= tokens::TokenType::TYPE_BEGIN &&
           tokens_.peek().getType() <= tokens::TokenType::TYPE_END)) {

        // Save the type argument name
        typeArgs.push_back(tokens_.peek().getLexeme());
        tokens_.advance(); // Consume the type name
      } else {
        error("Expected type name in generic type arguments");
        return nullptr;
      }

      // Continue if there's a comma
      if (!check(tokens::TokenType::COMMA)) {
        break;
      }
      tokens_.advance(); // Consume comma

    } while (!tokens_.isAtEnd());

    // Expect the closing ">"
    if (!consume(tokens::TokenType::GREATER,
                 "Expected '>' after generic type arguments")) {
      return nullptr;
    }

    // Expect the function call opening parenthesis
    if (!consume(tokens::TokenType::LEFT_PAREN,
                 "Expected '(' after generic type arguments")) {
      return nullptr;
    }

    // Parse function arguments
    std::vector<nodes::ExpressionPtr> args;
    if (!check(tokens::TokenType::RIGHT_PAREN)) {
      do {
        auto arg = parentVisitor_.parseExpression();
        if (!arg) {
          return nullptr;
        }
        args.push_back(std::move(arg));
      } while (match(tokens::TokenType::COMMA));
    }

    // Expect the closing ")"
    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after function arguments")) {
      return nullptr;
    }

    // Create the call expression node with type arguments
    nodes::ExpressionPtr callExpr = std::make_shared<nodes::CallExpressionNode>(
        expr->getLocation(), expr, std::move(args), std::move(typeArgs));

    // Continue parsing any additional postfix operations
    return parsePostfixOperations(callExpr);
  }

  // Parses postfix operations such as member access, array indexing, and
  // function calls.
  nodes::ExpressionPtr parsePostfixOperations(nodes::ExpressionPtr expr) {
    while (true) {
      // Handle member access: e.g. obj.property (for "this._width")
      if (match(tokens::TokenType::DOT)) {
        if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
          error("Expected property name after '.'");
          return nullptr;
        }
        // Advance and get the member token
        auto memberToken = tokens_.advance();
        std::string memberName = memberToken.getLexeme();

        // Create a MemberExpressionNode using the member token's location,
        // the current expression as the object, the member name, and false for
        // dot notation.
        expr = std::make_shared<nodes::MemberExpressionNode>(
            memberToken.getLocation(), // source location for the member
            expr,                      // object (e.g. "this")
            memberName,                // member name (e.g. "_width")
            false // isPointer flag (false for dot notation)
        );
      }
      // Handle array indexing: array[expression]
      else if (match(tokens::TokenType::LEFT_BRACKET)) {
        auto index = parentVisitor_.parseExpression();
        if (!index) {
          return nullptr;
        }
        if (!consume(tokens::TokenType::RIGHT_BRACKET,
                     "Expected ']' after array index")) {
          return nullptr;
        }
        expr = std::make_shared<nodes::IndexExpressionNode>(
            expr->getLocation(),
            expr, // the array expression
            index // the index expression
        );
      }
      // Handle function calls: func(arg1, arg2, ...)
      else if (match(tokens::TokenType::LEFT_PAREN)) {
        std::vector<nodes::ExpressionPtr> arguments;
        if (!check(tokens::TokenType::RIGHT_PAREN)) {
          do {
            auto arg = parentVisitor_.parseExpression();
            if (!arg) {
              return nullptr;
            }
            arguments.push_back(std::move(arg));
          } while (match(tokens::TokenType::COMMA));
        }
        if (!consume(tokens::TokenType::RIGHT_PAREN,
                     "Expected ')' after function arguments")) {
          return nullptr;
        }
        expr = std::make_shared<nodes::CallExpressionNode>(
            expr->getLocation(),
            expr,                // the callee expression
            std::move(arguments) // function arguments
        );
      } else {
        break; // No more postfix operations found.
      }
    }
    return expr;
  }

private:
  // Parses an array literal, e.g. [elem1, elem2, ...]
  nodes::ExpressionPtr parseArrayLiteral() {
    auto location = tokens_.previous().getLocation();
    std::vector<nodes::ExpressionPtr> elements;

    // Handle empty array literal.
    if (check(tokens::TokenType::RIGHT_BRACKET)) {
      tokens_.advance();
      return std::make_shared<nodes::ArrayLiteralNode>(location,
                                                       std::move(elements));
    }

    // Parse the array elements separated by commas.
    do {
      auto element = parentVisitor_.parseExpression();
      if (!element)
        return nullptr;
      elements.push_back(std::move(element));
    } while (match(tokens::TokenType::COMMA));

    if (!consume(tokens::TokenType::RIGHT_BRACKET,
                 "Expected ']' after array elements")) {
      return nullptr;
    }

    return std::make_shared<nodes::ArrayLiteralNode>(location,
                                                     std::move(elements));
  }

  // Utility: If the next token matches 'type', advance and return true.
  bool match(tokens::TokenType type) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    return false;
  }

  // Utility: Check if the next token is of type 'type'.
  bool check(tokens::TokenType type) const {
    return !tokens_.isAtEnd() && tokens_.peek().getType() == type;
  }

  // Utility: Consume a token of type 'type' or report an error with 'message'.
  bool consume(tokens::TokenType type, const std::string &message) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    error(message);
    return false;
  }

  // Report an error at the current token's location.
  void error(const std::string &message) {
    errorReporter_.error(tokens_.peek().getLocation(), message);
  }

  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &parentVisitor_;
};

} // namespace visitors
