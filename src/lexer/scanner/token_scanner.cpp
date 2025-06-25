/*****************************************************************************
 * File: token_scanner.cpp
 * Description: Implementation of main token scanner for TSPP lexical analysis
 *****************************************************************************/
#include "token_scanner.h"
#include "core/common/common_types.h"

namespace lexer {

/*****************************************************************************
 * Constructor Implementation
 *****************************************************************************/
TokenScanner::TokenScanner(std::shared_ptr<LexerState> state)
    : state_(state), identifierScanner_(state), numberScanner_(state),
      operatorScanner_(state), stringScanner_(state) {}

/*****************************************************************************
 * Public Methods Implementation
 *****************************************************************************/
tokens::Token TokenScanner::scanToken() {
  skipWhitespace();

  if (state_->isAtEnd()) {
    return makeEndToken();
  }

  if (checkComment()) {
    return scanToken();
  }

  char c = state_->getCurrentChar();

  // Handle string literals
  if (c == '"') {
    return stringScanner_.scanString();
  }

  // Handle character literals
  if (c == '\'') {
    return stringScanner_.scanCharacter();
  }

  // Handle numbers (including decimals)
  if (std::isdigit(c) || (c == '.' && std::isdigit(state_->peekNext()))) {
    return numberScanner_.scan();
  }

  // Handle identifiers and keywords
  if (std::isalpha(c) || c == '_') {
    return identifierScanner_.scan();
  }

  // Handle attributes
  if (c == '#') {
    return identifierScanner_.scanAttribute();
  }

  // Handle operators and other symbols
  return operatorScanner_.scan();
}

/*****************************************************************************
 * Private Helper Methods Implementation
 *****************************************************************************/
void TokenScanner::skipWhitespace() {
  while (!state_->isAtEnd()) {
    char c = state_->getCurrentChar();
    if (std::isspace(c)) {
      if (c == '\n') {
        state_->newLine();
      } else {
        state_->advance();
      }
    } else {
      break;
    }
  }
}

bool TokenScanner::checkComment() {
  if (state_->getCurrentChar() == '/') {
    if (state_->peekNext(1) == '/') {
      skipLineComment();
      return true;
    }
    if (state_->peekNext(1) == '*') {
      skipBlockComment();
      return true;
    }
  }
  return false;
}

void TokenScanner::skipLineComment() {
  while (!state_->isAtEnd() && state_->getCurrentChar() != '\n') {
    state_->advance();
  }
}

void TokenScanner::skipBlockComment() {
  state_->advance(); // Skip /
  state_->advance(); // Skip *

  while (!state_->isAtEnd()) {
    if (state_->getCurrentChar() == '*' && state_->peekNext(1) == '/') {
      state_->advance(); // Skip *
      state_->advance(); // Skip /
      return;
    }

    if (state_->getCurrentChar() == '\n') {
      state_->newLine();
    } else {
      state_->advance();
    }
  }
}

tokens::Token TokenScanner::makeEndToken() {
  core::SourceLocation location(state_->getLine(), state_->getColumn(),
                                 state_->getFileName());
  return tokens::Token(tokens::TokenType::END_OF_FILE, "", location);
}

} // namespace lexer