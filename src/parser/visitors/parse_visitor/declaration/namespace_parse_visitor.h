#pragma once
#include "core/diagnostics/error_reporter.h"
#include "ideclaration_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "tokens/stream/token_stream.h"
#include <vector>

namespace visitors {

class NamespaceParseVisitor {
public:
  NamespaceParseVisitor(tokens::TokenStream &tokens,
                        core::ErrorReporter &errorReporter,
                        IDeclarationVisitor &declVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        declVisitor_(declVisitor) {}

  /**
   * Parse a namespace declaration
   * namespace name { declarations... }
   */
  nodes::DeclPtr parseNamespaceDecl() {
    auto location = tokens_.peek().getLocation();

    // Expect 'namespace' keyword
    if (!consume(tokens::TokenType::NAMESPACE,
                 "Expected 'namespace' keyword")) {
      return nullptr;
    }

    // Parse namespace name
    if (!consume(tokens::TokenType::IDENTIFIER, "Expected namespace name")) {
      return nullptr;
    }
    std::string name = tokens_.previous().getLexeme();

    // Parse namespace body
    if (!consume(tokens::TokenType::LEFT_BRACE,
                 "Expected '{' after namespace name")) {
      return nullptr;
    }

    // Parse declarations inside the namespace
    std::vector<nodes::DeclPtr> declarations;
    while (!check(tokens::TokenType::RIGHT_BRACE) && !tokens_.isAtEnd()) {
      auto decl = declVisitor_.parseDeclaration();
      if (decl) {
        declarations.push_back(std::move(decl));
      } else {
        // Skip to the next declaration if there was an error
        synchronize();
      }
    }

    // Expect closing brace
    if (!consume(tokens::TokenType::RIGHT_BRACE,
                 "Expected '}' after namespace body")) {
      return nullptr;
    }

    return std::make_shared<nodes::NamespaceDeclNode>(
        name, std::move(declarations), location);
  }

private:
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IDeclarationVisitor &declVisitor_;

  // Helper methods for token handling
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

  // Skip tokens until we reach a position where a new declaration might start
  void synchronize() {
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
      case tokens::TokenType::INTERFACE:
      case tokens::TokenType::ENUM:
      case tokens::TokenType::NAMESPACE:
      case tokens::TokenType::TYPEDEF:
        return;
      default:
        break;
      }

      tokens_.advance();
    }
  }
};

} // namespace visitors