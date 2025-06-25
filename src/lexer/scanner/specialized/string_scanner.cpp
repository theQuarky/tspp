/*****************************************************************************
 * File: string_scanner.cpp
 * Description: Implementation of string and character literal scanning.
 *****************************************************************************/

#include "string_scanner.h"
#include <algorithm>
#include <cctype>

namespace lexer {

/*****************************************************************************
 * Constructor Implementation
 *****************************************************************************/
StringScanner::StringScanner(std::shared_ptr<LexerState> state)
    : ScannerBase(std::move(state)) {}

/*****************************************************************************
 * Public Methods Implementation
 *****************************************************************************/
tokens::Token StringScanner::scanString() {
  size_t start = state_->getPosition();

  advance(); // Skip opening quote

  // Process string contents
  while (!isAtEnd() && peek() != '"') {
    if (peek() == '\\') {
      std::string escape = scanEscapeSequence();
      if (escape.empty()) {
        return makeErrorToken("Invalid escape sequence");
      }
    } else if (!validateCharacter(peek())) {
      return makeErrorToken("Invalid character in string");
    } else {
      advance();
    }
  }

  // Check for unterminated string
  if (isAtEnd()) {
    return makeErrorToken("Unterminated string");
  }

  advance(); // Skip closing quote
  return makeToken(tokens::TokenType::STRING_LITERAL, start,
                   state_->getPosition() - start);
}

tokens::Token StringScanner::scanCharacter() {
  size_t start = state_->getPosition();
  advance(); // Skip opening quote

  // Process character content
  if (peek() == '\\') {
    std::string escape = scanEscapeSequence();
    if (escape.empty()) {
      return makeErrorToken("Invalid escape sequence");
    }
  } else if (!validateCharacter(peek())) {
    return makeErrorToken("Invalid character literal");
  } else {
    advance();
  }

  // Validate closing quote
  if (peek() != '\'') {
    return makeErrorToken("Invalid character literal");
  }

  advance(); // Skip closing quote
  return makeToken(tokens::TokenType::CHAR_LITERAL, start,
                   state_->getPosition() - start);
}

/*****************************************************************************
 * Private Helper Methods Implementation
 *****************************************************************************/
std::string StringScanner::scanEscapeSequence() {
  advance(); // Skip backslash

  // Handle escape sequences
  switch (peek()) {
  // Simple escape sequences
  case '\'':
  case '"':
  case '\\':
  case 'n':
  case 'r':
  case 't':
  case '0': {
    char c = peek();
    advance();
    return std::string(1, c);
  }
  // Hex escape sequence
  case 'x': {
    advance();
    std::string hex;
    for (int i = 0; i < 2 && !isAtEnd() && std::isxdigit(peek()); ++i) {
      hex += peek();
      advance();
    }
    return (hex.length() == 2 && validateEscapeSequence(hex)) ? hex : "";
  }
  // Unicode escape sequence
  case 'u': {
    advance();
    std::string unicode;
    for (int i = 0; i < 4 && !isAtEnd() && std::isxdigit(peek()); ++i) {
      unicode += peek();
      advance();
    }
    return (unicode.length() == 4 && validateEscapeSequence(unicode)) ? unicode
                                                                      : "";
  }
  default:
    return "";
  }
}

bool StringScanner::validateCharacter(char c) const {
  return c >= 32 && c <= 126; // Printable ASCII range
}

bool StringScanner::validateEscapeSequence(const std::string &sequence) const {
  // Check hex escape sequence
  if (sequence.length() == 2) {
    return std::all_of(sequence.begin(), sequence.end(), ::isxdigit);
  }
  // Check unicode escape sequence
  if (sequence.length() == 4) {
    return std::all_of(sequence.begin(), sequence.end(), ::isxdigit);
  }
  return false;
}

} // namespace lexer