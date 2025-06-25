#pragma once
#include "core/diagnostics/error_reporter.h"
#include "ideclaration_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"
#include "tokens/token_type.h"
#include <iostream>
#include <ostream>
#include <vector>

namespace visitors {

class InterfaceParseVisitor {
public:
  InterfaceParseVisitor(tokens::TokenStream &tokens,
                        core::ErrorReporter &errorReporter,
                        IDeclarationVisitor &declVisitor,
                        IExpressionVisitor &exprVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        declVisitor_(declVisitor), exprVisitor_(exprVisitor) {}

  /**
   * Parse an interface declaration
   * #zerocast? interface Name <T extends Constraint, U> extends Base {
   * members... }
   */
  nodes::DeclPtr parseInterfaceDecl() {
    auto location = tokens_.peek().getLocation();

    // Check for #zerocast attribute
    bool isZeroCast = false;
    if (tokens_.getCurrentToken().getLexeme() == "#zerocast") {
      isZeroCast = true;
      tokens_.advance();
    }
    std::cout << "current token: " << tokens_.getCurrentToken().getLexeme()
              << std::endl;
    // Expect 'interface' keyword
    if (!consume(tokens::TokenType::INTERFACE,
                 "Expected 'interface' keyword")) {
      return nullptr;
    }

    // Parse interface name
    if (!consume(tokens::TokenType::IDENTIFIER, "Expected interface name")) {
      return nullptr;
    }
    std::string name = tokens_.previous().getLexeme();

    // Check for generic parameters
    std::vector<nodes::TypePtr> genericParams;
    if (match(tokens::TokenType::LESS)) {
      // Parse generic parameter list
      do {
        if (!consume(tokens::TokenType::IDENTIFIER,
                     "Expected generic parameter name")) {
          return nullptr;
        }

        std::string paramName = tokens_.previous().getLexeme();
        auto paramLoc = tokens_.previous().getLocation();

        // Check for constraints (extends T)
        std::vector<nodes::TypePtr> constraints;
        if (match(tokens::TokenType::EXTENDS)) {
          // Parse constraint type
          auto constraintType = declVisitor_.parseType();
          if (!constraintType) {
            return nullptr;
          }
          constraints.push_back(std::move(constraintType));

          // Handle multiple constraints with &
          while (match(tokens::TokenType::AMPERSAND)) {
            auto additionalConstraint = declVisitor_.parseType();
            if (!additionalConstraint) {
              return nullptr;
            }
            constraints.push_back(std::move(additionalConstraint));
          }
        }

        genericParams.push_back(std::make_shared<nodes::GenericParamNode>(
            paramName, std::move(constraints), paramLoc));

      } while (match(tokens::TokenType::COMMA));

      if (!consume(tokens::TokenType::GREATER,
                   "Expected '>' after generic parameters")) {
        return nullptr;
      }
    }

    // Check for 'extends' keyword
    std::vector<nodes::TypePtr> extendedInterfaces;
    if (match(tokens::TokenType::EXTENDS)) {
      // Parse extended interfaces list
      do {
        auto type = declVisitor_.parseType();
        if (!type) {
          return nullptr;
        }
        extendedInterfaces.push_back(std::move(type));
      } while (match(tokens::TokenType::COMMA));
    }

    // Parse interface body
    if (!consume(tokens::TokenType::LEFT_BRACE,
                 "Expected '{' after interface declaration")) {
      return nullptr;
    }

    // Parse interface members
    std::vector<nodes::DeclPtr> members;
    while (!check(tokens::TokenType::RIGHT_BRACE) && !tokens_.isAtEnd()) {
      auto member = parseInterfaceMember();
      if (member) {
        members.push_back(std::move(member));
      } else {
        // Skip to the next member if there was an error
        synchronize();
      }
    }

    // Expect closing brace
    if (!consume(tokens::TokenType::RIGHT_BRACE,
                 "Expected '}' after interface body")) {
      return nullptr;
    }

    // Create appropriate interface node
    if (genericParams.empty()) {
      return std::make_shared<nodes::InterfaceDeclNode>(
          name, std::move(extendedInterfaces), std::move(members), isZeroCast,
          location);
    } else {
      return std::make_shared<nodes::GenericInterfaceDeclNode>(
          name, std::move(extendedInterfaces), std::move(members), isZeroCast,
          std::move(genericParams), location);
    }
  }

private:
  /**
   * Parse an interface member (method signature or property signature)
   */
  nodes::DeclPtr parseInterfaceMember() {
    auto location = tokens_.peek().getLocation();

    // Check for access modifier
    tokens::TokenType accessModifier =
        tokens::TokenType::PUBLIC; // Default to public
    if (match(tokens::TokenType::PUBLIC) || match(tokens::TokenType::PRIVATE) ||
        match(tokens::TokenType::PROTECTED)) {
      accessModifier = tokens_.previous().getType();
    }

    // Check for property getter/setter
    if (match(tokens::TokenType::GET)) {
      return parsePropertySignature(accessModifier, true, false, location);
    } else if (match(tokens::TokenType::SET)) {
      return parsePropertySignature(accessModifier, false, true, location);
    }

    // Parse method signature (no function keyword in interfaces)
    if (match(tokens::TokenType::IDENTIFIER)) {
      std::string methodName = tokens_.previous().getLexeme();

      // Parse parameter list
      if (!consume(tokens::TokenType::LEFT_PAREN,
                   "Expected '(' after method name")) {
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

      // Parse return type
      if (!consume(tokens::TokenType::COLON,
                   "Expected ':' after method parameters")) {
        return nullptr;
      }

      auto returnType = declVisitor_.parseType();
      if (!returnType) {
        return nullptr;
      }

      // Parse optional throws clause
      std::vector<nodes::TypePtr> throwsTypes;
      if (match(tokens::TokenType::THROWS)) {
        do {
          auto type = declVisitor_.parseType();
          if (!type) {
            return nullptr;
          }
          throwsTypes.push_back(std::move(type));
        } while (match(tokens::TokenType::COMMA));
      }

      // Interface methods end with semicolon
      if (!consume(tokens::TokenType::SEMICOLON,
                   "Expected ';' after method signature")) {
        return nullptr;
      }

      return std::make_shared<nodes::MethodSignatureNode>(
          methodName, accessModifier, std::move(parameters),
          std::move(returnType), std::move(throwsTypes), location);
    }

    error("Expected interface member (method or property)");
    return nullptr;
  }

  /**
   * Parse a property signature
   */
  nodes::DeclPtr parsePropertySignature(tokens::TokenType accessModifier,
                                        bool hasGetter, bool hasSetter,
                                        const core::SourceLocation &location) {
    // Parse property name
    if (!consume(tokens::TokenType::IDENTIFIER, "Expected property name")) {
      return nullptr;
    }
    std::string propertyName = tokens_.previous().getLexeme();

    // Parse parameters for setters
    if (hasSetter) {
      if (!consume(tokens::TokenType::LEFT_PAREN,
                   "Expected '(' after setter property name")) {
        return nullptr;
      }

      // Setter must have exactly one parameter
      auto param = parseParameter();
      if (!param) {
        return nullptr;
      }

      if (!consume(tokens::TokenType::RIGHT_PAREN,
                   "Expected ')' after setter parameter")) {
        return nullptr;
      }
    }

    // Parse type annotation
    if (!consume(tokens::TokenType::COLON,
                 "Expected ':' after property name")) {
      return nullptr;
    }

    auto type = declVisitor_.parseType();
    if (!type) {
      return nullptr;
    }

    // Interface properties end with semicolon
    if (!consume(tokens::TokenType::SEMICOLON,
                 "Expected ';' after property signature")) {
      return nullptr;
    }

    return std::make_shared<nodes::PropertySignatureNode>(
        propertyName, accessModifier, std::move(type), hasGetter, hasSetter,
        location);
  }

  /**
   * Parse a parameter declaration
   */
  nodes::ParamPtr parseParameter() {
    auto location = tokens_.peek().getLocation();

    // Check for parameter modifiers (ref, const)
    bool isRef = false;
    bool isConst = false;

    if (match(tokens::TokenType::REF)) {
      isRef = true;
    } else if (match(tokens::TokenType::CONST)) {
      isConst = true;
    }

    // Parse parameter name
    if (!consume(tokens::TokenType::IDENTIFIER, "Expected parameter name")) {
      return nullptr;
    }
    std::string paramName = tokens_.previous().getLexeme();

    // Parse type annotation
    if (!consume(tokens::TokenType::COLON,
                 "Expected ':' after parameter name")) {
      return nullptr;
    }

    auto type = declVisitor_.parseType();
    if (!type) {
      return nullptr;
    }

    // Parse optional default value
    nodes::ExpressionPtr defaultValue;
    if (match(tokens::TokenType::EQUALS)) {
      defaultValue = exprVisitor_.parseExpression();
      if (!defaultValue) {
        return nullptr;
      }
    }

    return std::make_shared<nodes::ParameterNode>(paramName, std::move(type),
                                                  std::move(defaultValue),
                                                  isRef, isConst, location);
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

  // Skip tokens until we might start a new interface member
  void synchronize() {
    tokens_.advance();

    while (!tokens_.isAtEnd()) {
      if (tokens_.previous().getType() == tokens::TokenType::SEMICOLON) {
        return;
      }

      switch (tokens_.peek().getType()) {
      case tokens::TokenType::PUBLIC:
      case tokens::TokenType::PRIVATE:
      case tokens::TokenType::PROTECTED:
      case tokens::TokenType::GET:
      case tokens::TokenType::SET:
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