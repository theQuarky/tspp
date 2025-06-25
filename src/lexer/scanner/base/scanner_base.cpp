/*****************************************************************************
 * File: scanner_base.cpp
 *
 * Description:
 *   Implementation of the base scanner class for lexical analysis.
 *   Provides core scanning functionality used by specialized scanners.
 *
 * Implementation Details:
 *   - Character matching and advancement
 *   - Token creation and error reporting
 *   - Position tracking and state management
 *
 * Error Handling:
 *   - Null state checking via assertions
 *   - Boundary checking for input
 *   - Error token creation with location info
 *
 * Notes:
 *   - All specialized scanners should inherit from this base
 *   - Maintains shared lexer state across all scanner instances
 *   - Thread-safe through shared_ptr usage
 *****************************************************************************/

#include "scanner_base.h"
#include <cassert>

namespace lexer {

/*****************************************************************************
 * Constructor Implementation
 *****************************************************************************/
ScannerBase::ScannerBase(std::shared_ptr<LexerState> state)
    : state_(std::move(state)) {
  assert(state_ && "Lexer state cannot be null");
}

/*****************************************************************************
 * Character Inspection Methods
 *****************************************************************************/
bool ScannerBase::match(char expected) {
  if (isAtEnd() || peek() != expected) {
    return false;
  }
  advance();
  return true;
}

char ScannerBase::peek() const { return state_->getCurrentChar(); }

char ScannerBase::peekNext(int n) const {
  if (n < 0) {
    return peek();
  }
  return state_->peekNext(n);
}

bool ScannerBase::isAtEnd() const { return state_->isAtEnd(); }

void ScannerBase::advance() {
  if (!isAtEnd()) {
    state_->advance();
  }
}

/*****************************************************************************
 * Token Creation Methods
 *****************************************************************************/
tokens::Token ScannerBase::makeToken(tokens::TokenType type, size_t start,
                                     size_t length) {
  if (length == 0) {
    return makeErrorToken("Invalid token length");
  }

  std::string_view source = state_->getSource();

  if (start + length > source.length()) {
    return makeErrorToken("Token position/length exceeds source length");
  }

  std::string_view lexeme = source.substr(start, length);

  core::SourceLocation location(state_->getLine(), state_->getColumn(),
                                 state_->getFileName());

  return tokens::Token(type, std::string(lexeme), location);
}

tokens::Token ScannerBase::makeErrorToken(const std::string &message) {
  core::SourceLocation location(state_->getLine(), state_->getColumn(),
                                 state_->getFileName());

  auto lexeme = state_->getCurrentLexeme();
  std::string errorLexeme =
      lexeme.empty() ? "" : std::string(lexeme.substr(0, 1));

  return tokens::Token::createError(errorLexeme, location, message);
}

} // namespace lexer