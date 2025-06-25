/*****************************************************************************
 * File: number_scanner.cpp
 *
 * Description:
 *   Implementation of numeric literal scanning functionality.
 *   Handles decimal, hex, binary, and floating-point numbers.
 *****************************************************************************/

#include "number_scanner.h"
#include "lexer/patterns/lexer_patterns.h"
#include <cctype>
#include <iostream>

namespace lexer {

/*****************************************************************************
 * Construction
 *****************************************************************************/
NumberScanner::NumberScanner(std::shared_ptr<LexerState> state)
    : ScannerBase(std::move(state)) {}

/*****************************************************************************
 * Public Interface Implementation
 *****************************************************************************/
tokens::Token NumberScanner::scan() {
  size_t start = state_->getPosition();

  // Check for hex or binary prefix
  if (peek() == '0') {
    advance(); // consume '0'

    if (peek() == 'x' || peek() == 'X') {
      advance(); // consume 'x'
      if (!scanHexDigits()) {
        return makeErrorToken("Invalid hexadecimal number");
      }
      size_t length = state_->getPosition() - start;
      return makeToken(tokens::TokenType::NUMBER, start, length);
    }

    if (peek() == 'b' || peek() == 'B') {
      advance(); // consume 'b'
      if (!scanBinaryDigits()) {
        return makeErrorToken("Invalid binary number");
      }
      size_t length = state_->getPosition() - start;
      return makeToken(tokens::TokenType::NUMBER, start, length);
    }

    // Regular number starting with 0
    if (std::isdigit(peek())) {
      if (!scanDigits()) {
        return makeErrorToken("Invalid decimal number");
      }
    }
  } else {
    // Regular decimal number
    if (!scanDigits()) {
      return makeErrorToken("Invalid decimal number");
    }
  }

  // Look for fractional part
  if (peek() == '.') {
    advance(); // consume '.'
    if (!scanDigits()) {
      return makeErrorToken("Invalid fractional part");
    }
  }

  // Look for exponent part
  if (peek() == 'e' || peek() == 'E') {
    advance(); // consume 'e'

    // Handle optional sign
    if (peek() == '+' || peek() == '-') {
      advance(); // consume sign
    }

    if (!scanDigits()) {
      return makeErrorToken("Invalid exponent part");
    }
  }

  size_t length = state_->getPosition() - start;
  return makeToken(tokens::TokenType::NUMBER, start, length);
}

/*****************************************************************************
 * Private Helper Methods
 *****************************************************************************/
bool NumberScanner::validateNumber(std::string_view lexeme) {
  return std::regex_match(lexeme.begin(), lexeme.end(),
                          LexerPatterns::getNumberPattern());
}

bool NumberScanner::scanDigits() {
  bool hasDigit = false;
  while (std::isdigit(peek()) || peek() == '_') {
    if (peek() != '_') {
      hasDigit = true;
    }
    advance();
  }
  return hasDigit;
}

bool NumberScanner::scanHexDigits() {
  bool hasDigit = false;
  while (std::isxdigit(peek()) || peek() == '_') {
    if (peek() != '_') {
      hasDigit = true;
    }
    advance();
  }
  return hasDigit;
}

bool NumberScanner::scanBinaryDigits() {
  bool hasDigit = false;
  while (peek() == '0' || peek() == '1' || peek() == '_') {
    if (peek() != '_') {
      hasDigit = true;
    }
    advance();
  }
  return hasDigit;
}

bool NumberScanner::scanExponent() {
  advance(); // Skip 'e' or 'E'

  // Handle optional sign
  if (peek() == '+' || peek() == '-') {
    advance();
  }

  return scanDigits();
}

bool NumberScanner::scanFraction() { return scanDigits(); }

} // namespace lexer