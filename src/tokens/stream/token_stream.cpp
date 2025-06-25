/*****************************************************************************
 * File: token_stream.cpp
 * Description: Implementation of TokenStream class providing sequential access
 * to token sequences with lookahead and matching capabilities.
 *****************************************************************************/

#include "token_stream.h"
#include "core/common/common_types.h"
#include <iostream>

namespace tokens {

/*****************************************************************************
 * Constructor Implementation
 *****************************************************************************/
TokenStream::TokenStream(std::vector<Token> tokens)
    : tokens_(std::move(tokens)), current_(0) {
  // Safety: Ensure stream always ends with EOF token
  if (tokens_.empty() || tokens_.back().getType() != TokenType::END_OF_FILE) {
    tokens_.push_back(
        Token(TokenType::END_OF_FILE, "", core::SourceLocation(0, 0)));
  }
}

/*****************************************************************************
 * Token Access Methods
 *****************************************************************************/

const Token &TokenStream::peek() const {
  // If at end or beyond, return the EOF token
  if (current_ >= tokens_.size() ||
      (current_ < tokens_.size() &&
       tokens_[current_].getType() == TokenType::END_OF_FILE)) {
    return tokens_.back();
  }
  return tokens_[current_];
}

const Token &TokenStream::peekNext(int n) const {
  // Handle negative lookhead as current position
  if (n <= 0) {
    n = 1;
  }
  // Return EOF token if next position beyond stream bounds
  if (current_ + n >= tokens_.size()) {
    return tokens_.back();
  }
  return tokens_[current_ + n];
}

const Token &TokenStream::previous() const {
  // Handle boundary cases
  if (current_ <= 0) {
    return tokens_[0]; // At start: return first token
  }
  if (current_ >= tokens_.size()) {
    return tokens_[tokens_.size() - 2]; // Return last real token, not EOF
  }
  return tokens_[current_ - 1];
}

Token TokenStream::advance() {
  // If already at end, don't advance further
  if (isAtEnd()) {
    return tokens_.back();
  }

  // Otherwise, move forward and return the token we were at
  Token token = tokens_[current_];
  current_++;
  return token;
}

/*****************************************************************************
 * Stream State Methods
 *****************************************************************************/

bool TokenStream::isAtEnd() const {
  // Simple end check: either we're at the EOF token or beyond bounds
  return current_ >= tokens_.size() - 1 ||
         (current_ < tokens_.size() &&
          tokens_[current_].getType() == TokenType::END_OF_FILE);
}

bool TokenStream::check(TokenType type) const {
  // Explicitly check for EOF case
  if (current_ >= tokens_.size()) {
    return type == TokenType::END_OF_FILE;
  }

  return peek().getType() == type;
}

bool TokenStream::match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }
  return false;
}

bool TokenStream::matchAny(const std::vector<TokenType> &types) {
  // Try to match any of the provided types
  for (const auto &type : types) {
    if (match(type)) {
      return true;
    }
  }
  return false;
}

/*****************************************************************************
 * Position Management
 *****************************************************************************/
void TokenStream::setPosition(size_t position) {
  // Ensure position stays within valid bounds
  if (position < tokens_.size() - 1) { // Subtract 1 to avoid EOF
    current_ = position;
  } else {
    current_ = tokens_.size() - 2; // Set to last real token, not EOF
  }
}

size_t TokenStream::getCurrentPosition() const { return current_; }

// New method: Save current position
size_t TokenStream::savePosition() const { return current_; }

// New method: Restore saved position
void TokenStream::restorePosition(size_t position) { setPosition(position); }

/*****************************************************************************
 * Error Recovery
 *****************************************************************************/
void TokenStream::synchronize() {
  // Skip the token where error occurred, if not at end
  if (!isAtEnd()) {
    advance();
  }

  // Continue until we find a synchronization point
  while (!isAtEnd()) {
    // Synchronize at statement boundaries (semicolons)
    if (previous().getType() == TokenType::SEMICOLON) {
      return;
    }

    // Or at the start of major declarations/statements
    switch (peek().getType()) {
    case TokenType::FUNCTION:    // Function declaration
    case TokenType::LET:         // Variable declaration
    case TokenType::FOR:         // For loop
    case TokenType::IF:          // If statement
    case TokenType::WHILE:       // While loop
    case TokenType::RETURN:      // Return statement
    case TokenType::RIGHT_BRACE: // End of block - added for better recovery
      return;
    default:
      advance(); // Skip tokens until sync point found
    }
  }
}

Token TokenStream::getCurrentToken() {
  if (current_ < tokens_.size()) {
    return tokens_[current_];
  }
  return tokens_.back(); // Return EOF if beyond bounds
}

} // namespace tokens