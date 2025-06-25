#pragma once
#include "core/diagnostics/error_reporter.h"
#include "ideclaration_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"
#include <iostream>
#include <vector>

namespace visitors {

class EnumParseVisitor {
public:
  EnumParseVisitor(tokens::TokenStream &tokens,
                   core::ErrorReporter &errorReporter,
                   IDeclarationVisitor &declVisitor,
                   IExpressionVisitor &exprVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        declVisitor_(declVisitor), exprVisitor_(exprVisitor) {}

  /**
   * Parse an enum declaration
   * enum Name : Type? { Member1 = value, Member2, ... }
   */
  nodes::DeclPtr parseEnumDecl() {
    auto location = tokens_.peek().getLocation();

    // Expect 'enum' keyword
    if (!consume(tokens::TokenType::ENUM, "Expected 'enum' keyword")) {
      return nullptr;
    }

    // Parse enum name
    if (!consume(tokens::TokenType::IDENTIFIER, "Expected enum name")) {
      return nullptr;
    }
    std::string name = tokens_.previous().getLexeme();

    // Parse optional underlying type
    nodes::TypePtr underlyingType;
    if (match(tokens::TokenType::COLON)) {
      underlyingType = declVisitor_.parseType();
      if (!underlyingType) {
        return nullptr;
      }
    }

    // Parse enum body
    if (!consume(tokens::TokenType::LEFT_BRACE,
                 "Expected '{' after enum declaration")) {
      return nullptr;
    }

// Parse enum members
std::vector<nodes::EnumMemberPtr> members;
while (!check(tokens::TokenType::RIGHT_BRACE) && !tokens_.isAtEnd()) {
  // Skip any semicolons
  while (check(tokens::TokenType::SEMICOLON)) {
    tokens_.advance();
  }
  
  // If we've reached the closing brace, we're done
  if (check(tokens::TokenType::RIGHT_BRACE)) {
    break;
  }
  
  // Parse the enum member
  auto member = parseEnumMember();
  if (!member) {
    // Error already reported in parseEnumMember
    return nullptr;
  }
  members.push_back(std::move(member));
  
  // Check for comma or semicolon or right brace
  if (check(tokens::TokenType::COMMA)) {
    tokens_.advance(); // Consume comma
  } else if (!check(tokens::TokenType::SEMICOLON) && 
             !check(tokens::TokenType::RIGHT_BRACE)) {
    // If no comma, semicolon, or right brace, that's an error
    error("Expected ',' or ';' after enum member");
    return nullptr;
  }
}

    // Expect closing brace
    if (!consume(tokens::TokenType::RIGHT_BRACE,
                 "Expected '}' after enum body")) {
      return nullptr;
    }

    return std::make_shared<nodes::EnumDeclNode>(
        name, std::move(underlyingType), std::move(members), location);
  }

private:
  /**
   * Parse an enum member
   * Member = value
   */
  nodes::EnumMemberPtr parseEnumMember() {
    auto location = tokens_.peek().getLocation();

    // Parse member name
    if (!consume(tokens::TokenType::IDENTIFIER, "Expected enum member name")) {
      return nullptr;
    }
    std::string memberName = tokens_.previous().getLexeme();

    // Parse optional explicit value
    nodes::ExpressionPtr value;
    if (match(tokens::TokenType::EQUALS)) {
      value = exprVisitor_.parseExpression();
      if (!value) {
        return nullptr;
      }
    }

    return std::make_shared<nodes::EnumMemberNode>(memberName, std::move(value),
                                                   location);
  }

  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IDeclarationVisitor &declVisitor_;
  IExpressionVisitor &exprVisitor_;

  // Helper methods
  bool consume(tokens::TokenType type, const std::string &message) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    error(message);
    return false;
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

  void error(const std::string &message) {
    errorReporter_.error(tokens_.peek().getLocation(), message);
  }

  // Skip tokens until we might start a new enum member
  void synchronize() {
    tokens_.advance();

    while (!tokens_.isAtEnd()) {
      if (tokens_.previous().getType() == tokens::TokenType::COMMA) {
        return;
      }

      switch (tokens_.peek().getType()) {
      case tokens::TokenType::IDENTIFIER:
      case tokens::TokenType::RIGHT_BRACE:
        return;
      default:
        break;
      }

      tokens_.advance();
    }
  }
};

} // namespace visitors