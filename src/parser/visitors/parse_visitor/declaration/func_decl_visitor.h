#pragma once
#include "core/diagnostics/error_reporter.h"
#include "ideclaration_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "parser/visitors/parse_visitor/statement/istatement_visitor.h"
#include "tokens/stream/token_stream.h"
#include <iostream>
#include <iterator>
#include <ostream>

namespace visitors {

class DeclarationParseVisitor;
class StatementParseVisitor;

class FunctionDeclarationVisitor {
public:
  FunctionDeclarationVisitor(tokens::TokenStream &tokens,
                             core::ErrorReporter &errorReporter,
                             IExpressionVisitor &exprVisitor,
                             IDeclarationVisitor &declVisitor,
                             IStatementVisitor &stmtVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        exprVisitor_(exprVisitor), declVisitor_(declVisitor),
        stmtVisitor_(stmtVisitor) {}

  nodes::DeclPtr
  parseFuncDecl(const std::vector<tokens::TokenType> &initialModifiers = {}) {
    auto location = tokens_.peek().getLocation();

    // Use the modifiers passed from DeclarationParseVisitor
    std::vector<tokens::TokenType> modifiers = initialModifiers;

    // Parse 'function' keyword
    if (!match(tokens::TokenType::FUNCTION)) {
      error("Expected 'function' keyword");
      return nullptr;
    }

    // Parse function name and generic parameters
    if (!match(tokens::TokenType::IDENTIFIER)) {
      error("Expected function name");
      return nullptr;
    }
    auto name = tokens_.previous().getLexeme();

    // Parse generic parameters if present
    std::vector<nodes::TypePtr> genericParams;
    if (match(tokens::TokenType::LESS)) {
      do {
        if (!match(tokens::TokenType::IDENTIFIER)) {
          error("Expected generic parameter name");
          return nullptr;
        }
        auto paramName = tokens_.previous().getLexeme();
        auto paramLocation = tokens_.previous().getLocation();

        // Check for constraints (extends TypeConstraint)
        std::vector<nodes::TypePtr> constraints;
        if (match(tokens::TokenType::EXTENDS)) {
          // Parse the constraint type
          auto constraint = parseTypeConstraint();
          if (!constraint)
            return nullptr;
          constraints.push_back(constraint);
        }

        // Create a GenericParamNode instead of NamedTypeNode
        genericParams.push_back(std::make_shared<nodes::GenericParamNode>(
            paramName, std::move(constraints), paramLocation));

      } while (match(tokens::TokenType::COMMA));

      if (!consume(tokens::TokenType::GREATER,
                   "Expected '>' after generic parameters")) {
        return nullptr;
      }
    }

    // Parse regular parameters
    if (!consume(tokens::TokenType::LEFT_PAREN,
                 "Expected '(' after function name")) {
      return nullptr;
    }

    std::vector<nodes::ParamPtr> parameters;
    if (!check(tokens::TokenType::RIGHT_PAREN)) {
      do {
        auto param = parseParameter();
        if (!param)
          return nullptr;
        parameters.push_back(std::move(param));
      } while (match(tokens::TokenType::COMMA));
    }

    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after parameters")) {
      return nullptr;
    }

    // Parse return type
    nodes::TypePtr returnType;
    if (match(tokens::TokenType::COLON)) {
      returnType = declVisitor_.parseType();
      if (!returnType)
        return nullptr;
    }

    // Parse where clause if present
    std::vector<std::pair<std::string, nodes::TypePtr>> constraints;
    if (match(tokens::TokenType::WHERE)) {
      do {
        if (!match(tokens::TokenType::IDENTIFIER)) {
          error("Expected type parameter name in constraint");
          return nullptr;
        }
        auto paramName = tokens_.previous().getLexeme();

        if (!consume(tokens::TokenType::COLON,
                     "Expected ':' after type parameter")) {
          return nullptr;
        }

        auto constraintType = declVisitor_.parseType();
        if (!constraintType)
          return nullptr;

        constraints.emplace_back(paramName, std::move(constraintType));
      } while (match(tokens::TokenType::COMMA));
    }

    // Parse throws clause
    std::vector<nodes::TypePtr> throwsTypes;
    if (match(tokens::TokenType::THROWS)) {
      do {
        auto throwType = declVisitor_.parseType();
        if (!throwType)
          return nullptr;
        throwsTypes.push_back(std::move(throwType));
      } while (match(tokens::TokenType::COMMA));
    }

    // Parse function body
    if (!match(tokens::TokenType::LEFT_BRACE)) {
      error("Expected '{' before function body");
      return nullptr;
    }
    auto body = stmtVisitor_.parseBlock();
    if (!body)
      return nullptr;

    // Create appropriate node based on whether it's generic
    if (!genericParams.empty()) {
      return std::make_shared<nodes::GenericFunctionDeclNode>(
          name, std::move(genericParams), std::move(parameters),
          std::move(returnType), std::move(constraints), std::move(throwsTypes),
          std::move(modifiers), std::move(body),
          false, // isAsync
          location);
    } else {
      return std::make_shared<nodes::FunctionDeclNode>(
          name, std::move(parameters), std::move(returnType),
          std::move(throwsTypes), std::move(modifiers), std::move(body),
          false, // isAsync
          location);
    }
  }

  // Helper method to parse type constraints
  nodes::TypePtr parseTypeConstraint() {
    auto location = tokens_.peek().getLocation();

    // Check for built-in constraints
    if (match(tokens::TokenType::IDENTIFIER)) {
      std::string constraintName = tokens_.previous().getLexeme();

      // Handle built-in constraints like "number", "comparable", etc.
      if (nodes::isValidBuiltinConstraint(constraintName)) {
        return std::make_shared<nodes::BuiltinConstraintNode>(
            constraintName, tokens_.previous().getLocation());
      }

      // Not a built-in constraint, so it must be a user-defined type
      // Create a named type node
      return std::make_shared<nodes::NamedTypeNode>(
          constraintName, tokens_.previous().getLocation());
    }

    // If it's not a simple identifier, try to parse it as a more complex type
    return declVisitor_.parseType();
  }

private:
  nodes::ParamPtr parseParameter() {
    auto location = tokens_.peek().getLocation();

    // Parse parameter modifiers (ref/const)
    bool isRef = false;
    bool isConst = false;

    // Check for ref modifier
    if (match(tokens::TokenType::REF)) {
      isRef = true;
    }
    // You can add const parsing here later if needed
    else if (match(tokens::TokenType::CONST)) {
      isConst = true;
    }

    // Parse parameter name
    if (!match(tokens::TokenType::IDENTIFIER)) {
      error("Expected parameter name");
      return nullptr;
    }
    auto name = tokens_.previous().getLexeme();

    // Parse parameter type
    if (!consume(tokens::TokenType::COLON,
                 "Expected ':' after parameter name")) {
      return nullptr;
    }

    auto type = declVisitor_.parseType();
    if (!type) {
      return nullptr;
    }

    // Parse optional default value
    nodes::ExpressionPtr defaultValue = nullptr;
    if (match(tokens::TokenType::EQUALS)) {
      // Note: ref parameters typically don't have default values
      // but we'll allow it for flexibility
      defaultValue = exprVisitor_.parseExpression();
      if (!defaultValue) {
        error("Invalid default value expression");
        return nullptr;
      }
    }

    // Create parameter node with ref/const flags
    return std::make_shared<nodes::ParameterNode>(
        name, std::move(type), std::move(defaultValue),
        isRef,   // Now passing the ref flag
        isConst, // Now passing the const flag
        location);
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

  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &exprVisitor_;
  IDeclarationVisitor &declVisitor_;
  IStatementVisitor &stmtVisitor_;
};

} // namespace visitors