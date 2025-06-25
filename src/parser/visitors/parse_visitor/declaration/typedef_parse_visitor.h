#pragma once
#include "core/diagnostics/error_reporter.h"
#include "ideclaration_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "tokens/stream/token_stream.h"

namespace visitors {

class TypedefParseVisitor {
public:
  TypedefParseVisitor(tokens::TokenStream &tokens,
                      core::ErrorReporter &errorReporter,
                      IDeclarationVisitor &declVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        declVisitor_(declVisitor) {}

  /**
   * Parse a typedef declaration
   * typedef Name = Type;
   */
  nodes::DeclPtr parseTypedefDecl() {
    auto location = tokens_.peek().getLocation();

    // Expect 'typedef' keyword
    if (!consume(tokens::TokenType::TYPEDEF, "Expected 'typedef' keyword")) {
      return nullptr;
    }

    // Parse type alias name
    if (!consume(tokens::TokenType::IDENTIFIER, "Expected type alias name")) {
      return nullptr;
    }
    std::string name = tokens_.previous().getLexeme();

    // Expect equals sign
    if (!consume(tokens::TokenType::EQUALS,
                 "Expected '=' after type alias name")) {
      return nullptr;
    }

    // Parse the aliased type
    auto aliasedType = declVisitor_.parseType();
    if (!aliasedType) {
      return nullptr;
    }

    // Expect semicolon
    if (!consume(tokens::TokenType::SEMICOLON,
                 "Expected ';' after typedef declaration")) {
      return nullptr;
    }

    return std::make_shared<nodes::TypedefDeclNode>(
        name, std::move(aliasedType), location);
  }

private:
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IDeclarationVisitor &declVisitor_;

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
};

} // namespace visitors