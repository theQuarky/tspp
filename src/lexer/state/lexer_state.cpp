/*****************************************************************************
 * File: lexer_state.cpp
 * Description: Implementation of LexerState class providing source code
 *navigation and token collection functionality.
 *****************************************************************************/

#include "lexer_state.h"

namespace lexer {

/*****************************************************************************
 * Source Inspection Methods
 *****************************************************************************/

std::string_view LexerState::getCurrentLexeme() const {
  if (position_ >= source_.length()) {
    return std::string_view();
  }
  return std::string_view(source_).substr(position_);
}

char LexerState::getCurrentChar() const {
  if (position_ >= source_.length() || position_ < 0) {
    return '\0';
  }
  return source_[position_];
}

char LexerState::peekNext(int n) const {
  // Handle negative lookhead
  if (n < 0) {
    return getCurrentChar();
  }

  // Calculate peek position and bounds check
  size_t peek_pos = position_ + n;
  if (peek_pos >= source_.length()) {
    return '\0';
  }
  return source_[peek_pos];
}

bool LexerState::match(char expected) {
  if (isAtEnd() || getCurrentChar() != expected) {
    return false;
  }
  advance();
  return true;
}

std::string_view LexerState::getLineContent() const {
  // Find start of current line
  size_t lineStart = source_.rfind('\n', position_);
  if (lineStart == std::string::npos) {
    lineStart = 0;
  } else {
    lineStart++; // Skip newline character
  }

  // Find end of current line
  size_t lineEnd = source_.find('\n', position_);
  if (lineEnd == std::string::npos) {
    lineEnd = source_.length();
  }

  return std::string_view(source_).substr(lineStart, lineEnd - lineStart);
}

/*****************************************************************************
 * State Management Methods
 *****************************************************************************/

void LexerState::advance(size_t count) {
  if (count == 0)
    return;

  // Advance position and track columns, handling tabs
  for (size_t i = 0; i < count && !isAtEnd(); ++i) {
    if (getCurrentChar() == '\t') {
      column_ += 4; // Use 4 spaces per tab
    } else {
      column_++;
    }
    position_++;
  }
}

void LexerState::newLine() {
  if (position_ >= source_.length()) {
    return;
  }

  // Update line tracking
  line_++;
  column_ = 1;
  position_++;

  // Add implicit semicolon if needed (automatic semicolon insertion)
  if (!tokens_.empty()) {
    const auto &lastToken = tokens_.back();
    if (lastToken.getType() != tokens::TokenType::SEMICOLON &&
        lastToken.getType() != tokens::TokenType::LEFT_BRACE &&
        lastToken.getType() != tokens::TokenType::RIGHT_BRACE) {

      addToken(tokens::Token(tokens::TokenType::SEMICOLON, ";",
                             core::SourceLocation(line_ - 1, column_)));
    }
  }
}

void LexerState::addToken(tokens::Token token) {
  // Validate token location
  assert(token.getLocation().getLine() <= line_);
  assert(token.getLocation().getColumn() <= column_);

  tokens_.push_back(std::move(token));
}

void LexerState::reset() {
  position_ = 0;
  line_ = 1;
  column_ = 1;
  tokens_.clear();
}

/*****************************************************************************
 * State Query Methods
 *****************************************************************************/

const tokens::Token &LexerState::getLastToken() const {
  assert(!tokens_.empty() && "Cannot get last token from empty collection");
  return tokens_.back();
}

} // namespace lexer