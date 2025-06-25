#pragma once
#include "core/common/common_types.h"
#include "token_type.h"
#include <optional>
#include <string>

namespace tokens {

/**
 * Class representing a token with type, lexeme, and location information
 */
class Token {
public:
  /**
   * Regular token constructor
   */
  Token(TokenType type, std::string lexeme, core::SourceLocation location)
      : type_(type), lexeme_(std::move(lexeme)),
        location_(std::move(location)) {}

  /**
   * Factory method for creating error tokens
   */
  static Token createError(std::string lexeme, core::SourceLocation location,
                           std::string errorMessage) {
    Token token(TokenType::ERROR_TOKEN, std::move(lexeme), std::move(location));
    token.errorMessage_ = std::move(errorMessage);
    return token;
  }

  /**
   * Token information access
   */
  TokenType getType() const { return type_; }
  const std::string &getLexeme() const { return lexeme_; }
  const core::SourceLocation &getLocation() const { return location_; }
  const std::optional<std::string> &getErrorMessage() const {
    return errorMessage_;
  }

  /**
   * Token classification methods
   */
  bool isError() const { return type_ == TokenType::ERROR_TOKEN; }
  bool isEOF() const { return type_ == TokenType::END_OF_FILE; }

  // Token category checks using token_type.h functions
  bool isDeclaration() const { return tokens::isDeclaration(type_); }
  bool isType() const { return tokens::isType(type_); }
  bool isControlFlow() const { return tokens::isControlFlow(type_); }
  bool isClassRelated() const { return tokens::isClassRelated(type_); }
  bool isOperator() const { return tokens::isOperator(type_); }
  bool isLiteral() const { return tokens::isLiteral(type_); }
  bool isDelimiter() const { return tokens::isDelimiter(type_); }
  bool isSpecial() const { return tokens::isSpecial(type_); }

private:
  TokenType type_;                          // Type of token
  std::string lexeme_;                      // Actual text of token
  core::SourceLocation location_;                 // Position in source
  std::optional<std::string> errorMessage_; // Error information if any
};

} // namespace tokens