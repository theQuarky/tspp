#include "expression_parse_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "tokens/token_type.h"
#include <iostream>
#include <ostream>

namespace visitors {

ExpressionParseVisitor::ExpressionParseVisitor(
    tokens::TokenStream &tokens, core::ErrorReporter &errorReporter,
    IDeclarationVisitor *declVisitor, IStatementVisitor *stmtVisitor)
    : tokens_(tokens), errorReporter_(errorReporter), declVisitor_(declVisitor),
      stmtVisitor_(stmtVisitor), binaryVisitor_(tokens, errorReporter, *this),
      unaryVisitor_(tokens, errorReporter, *this),
      primaryVisitor_(tokens, errorReporter, *this),
      callVisitor_(tokens, errorReporter, *this),
      castVisitor_(tokens, errorReporter, *this) {}

nodes::ExpressionPtr ExpressionParseVisitor::parseExpression() {
  try {
    // Start with lowest precedence
    return parseAssignment();
  } catch (const std::exception &e) {
    errorReporter_.error(tokens_.peek().getLocation(),
                         std::string("Error parsing expression: ") + e.what());
    return nullptr;
  }
}

nodes::ExpressionPtr ExpressionParseVisitor::parseAssignment() {
  auto expr = parseComparison(); // Changed from parseAdditive to handle
                                 // comparison operators
  if (!expr)
    return nullptr;

  if (match(tokens::TokenType::EQUALS) ||
      match(tokens::TokenType::PLUS_EQUALS) ||
      match(tokens::TokenType::MINUS_EQUALS) ||
      match(tokens::TokenType::STAR_EQUALS) ||
      match(tokens::TokenType::SLASH_EQUALS)) {

    auto op = tokens_.previous().getType();
    auto value = parseAssignment();
    if (!value)
      return nullptr;

    return std::make_shared<nodes::AssignmentExpressionNode>(
        expr->getLocation(), op, expr, value);
  }

  return expr;
}

nodes::ExpressionPtr ExpressionParseVisitor::parseComparison() {
  auto expr = parseAdditive();
  if (!expr)
    return nullptr;

  while (match(tokens::TokenType::LESS) ||
         match(tokens::TokenType::LESS_EQUALS) ||
         match(tokens::TokenType::GREATER) ||
         match(tokens::TokenType::GREATER_EQUALS) ||
         match(tokens::TokenType::EQUALS_EQUALS) ||
         match(tokens::TokenType::EXCLAIM_EQUALS)) {

    auto op = tokens_.previous().getType();
    auto right = parseAdditive();
    if (!right)
      return nullptr;

    expr = std::make_shared<nodes::BinaryExpressionNode>(expr->getLocation(),
                                                         op, expr, right);
  }

  return expr;
}

nodes::ExpressionPtr ExpressionParseVisitor::parseAdditive() {
  auto expr = parseMultiplicative();
  if (!expr)
    return nullptr;

  while (match(tokens::TokenType::PLUS) || match(tokens::TokenType::MINUS)) {
    auto op = tokens_.previous().getType();
    auto right = parseMultiplicative();
    if (!right)
      return nullptr;

    expr = std::make_shared<nodes::BinaryExpressionNode>(expr->getLocation(),
                                                         op, expr, right);
  }

  return expr;
}

nodes::ExpressionPtr ExpressionParseVisitor::parseMultiplicative() {
  auto expr = parseUnary();
  if (!expr)
    return nullptr;

  while (match(tokens::TokenType::STAR) || match(tokens::TokenType::SLASH) ||
         match(tokens::TokenType::PERCENT)) {
    auto op = tokens_.previous().getType();
    auto right = parseUnary();
    if (!right)
      return nullptr;

    expr = std::make_shared<nodes::BinaryExpressionNode>(expr->getLocation(),
                                                         op, expr, right);
  }

  return expr;
}

nodes::ExpressionPtr ExpressionParseVisitor::parsePrimary() {
  return primaryVisitor_.parsePrimary();
}

nodes::ExpressionPtr ExpressionParseVisitor::parseUnary() {
  return unaryVisitor_.parseUnary();
}

nodes::TypePtr ExpressionParseVisitor::parseType() {
  auto location = tokens_.peek().getLocation();
  std::string typeName;

  // Handle primitive types
  if (match(tokens::TokenType::VOID)) {
    typeName = "void";
  } else if (match(tokens::TokenType::INT)) {
    typeName = "int";
  } else if (match(tokens::TokenType::FLOAT)) {
    typeName = "float";
  } else if (match(tokens::TokenType::BOOLEAN)) {
    typeName = "bool";
  } else if (match(tokens::TokenType::STRING)) {
    typeName = "string";
  }
  // Handle user-defined types (identifiers)
  else if (match(tokens::TokenType::IDENTIFIER)) {
    typeName = tokens_.previous().getLexeme();
  } else {
    error("Expected type name");
    return nullptr;
  }

  return std::make_shared<nodes::NamedTypeNode>(typeName, location);
}

nodes::ExpressionPtr ExpressionParseVisitor::parseNewExpression() {
  auto location = tokens_.previous().getLocation();

  // Parse constructor name (class name)
  if (!match(tokens::TokenType::IDENTIFIER)) {
    error("Expected class name after 'new'");
    return nullptr;
  }

  std::string className = tokens_.previous().getLexeme();

  // Parse constructor arguments
  if (!consume(tokens::TokenType::LEFT_PAREN,
               "Expected '(' after class name")) {
    return nullptr;
  }

  std::vector<nodes::ExpressionPtr> arguments;
  if (!check(tokens::TokenType::RIGHT_PAREN)) {
    do {
      auto arg = parseExpression();
      if (!arg)
        return nullptr;
      arguments.push_back(std::move(arg));
    } while (match(tokens::TokenType::COMMA));
  }

  if (!consume(tokens::TokenType::RIGHT_PAREN,
               "Expected ')' after constructor arguments")) {
    return nullptr;
  }

  return std::make_shared<nodes::NewExpressionNode>(location, className,
                                                    std::move(arguments));
}

// Add to ExpressionParseVisitor class
nodes::ExpressionPtr ExpressionParseVisitor::parseFunctionExpression() {
  auto location = tokens_.peek().getLocation();

  // Consume 'function' keyword
  if (!consume(tokens::TokenType::FUNCTION,
               "Expected 'function' keyword in function expression")) {
    return nullptr;
  }

  // Parse parameter list
  if (!consume(tokens::TokenType::LEFT_PAREN,
               "Expected '(' after 'function'")) {
    return nullptr;
  }

  std::vector<nodes::ParamPtr> parameters;
  if (!check(tokens::TokenType::RIGHT_PAREN)) {
    do {
      auto param = parseParameter();
      if (!param) {
        return nullptr;
      }
      parameters.push_back(std::move(param));
    } while (match(tokens::TokenType::COMMA));
  }

  if (!consume(tokens::TokenType::RIGHT_PAREN,
               "Expected ')' after parameters")) {
    return nullptr;
  }

  // Parse optional return type
  nodes::TypePtr returnType;
  if (match(tokens::TokenType::COLON)) {
    returnType = declVisitor_->parseType();
    if (!returnType) {
      return nullptr;
    }
  }

  // Parse function body
  if (!consume(tokens::TokenType::LEFT_BRACE,
               "Expected '{' before function body")) {
    return nullptr;
  }

  auto body = stmtVisitor_->parseBlock();
  if (!body) {
    return nullptr;
  }

  // Create a function expression node
  return std::make_shared<nodes::FunctionExpressionNode>(
      std::move(parameters), std::move(returnType), std::move(body), location);
}
// Add to ExpressionParseVisitor class in expression_parse_visitor.cpp
nodes::ParamPtr ExpressionParseVisitor::parseParameter() {
  auto location = tokens_.peek().getLocation();

  // Parse parameter modifiers (ref/const)
  bool isRef = false;
  bool isConst = false;

  if (match(tokens::TokenType::REF)) {
    isRef = true;
  } else if (match(tokens::TokenType::CONST)) {
    isConst = true;
  }

  // Parse parameter name
  if (!match(tokens::TokenType::IDENTIFIER)) {
    error("Expected parameter name");
    return nullptr;
  }
  auto name = tokens_.previous().getLexeme();

  // Parse parameter type
  if (!consume(tokens::TokenType::COLON, "Expected ':' after parameter name")) {
    return nullptr;
  }

  // Check if declVisitor_ is null before calling parseType()
  if (!declVisitor_) {
    error("Internal error: Type parser is not available");
    return nullptr;
  }

  auto type = declVisitor_->parseType();
  if (!type) {
    return nullptr;
  }

  // Parse optional default value
  nodes::ExpressionPtr defaultValue = nullptr;
  if (match(tokens::TokenType::EQUALS)) {
    defaultValue = parseExpression();
    if (!defaultValue) {
      error("Invalid default value expression");
      return nullptr;
    }
  }

  // Create parameter node with ref/const flags
  return std::make_shared<nodes::ParameterNode>(
      name, std::move(type), std::move(defaultValue), isRef, isConst, location);
}

bool ExpressionParseVisitor::match(tokens::TokenType type) {
  if (check(type)) {
    tokens_.advance();
    return true;
  }
  return false;
}

bool ExpressionParseVisitor::check(tokens::TokenType type) const {
  return !tokens_.isAtEnd() && tokens_.peek().getType() == type;
}

void ExpressionParseVisitor::error(const std::string &message) {
  errorReporter_.error(tokens_.peek().getLocation(), message);
}

} // namespace visitors