/*****************************************************************************
 * File: lexer.cpp
 * Description: Implementation of main lexer for TSPP language
 *****************************************************************************/

#include "lexer.h"

namespace lexer {

Lexer::Lexer(std::string source, std::string filename)
    : state_(
          std::make_shared<LexerState>(std::move(source), std::move(filename))),
      scanner_(state_) {}

std::vector<tokens::Token> Lexer::tokenize() {
  while (!state_->isAtEnd()) {
    tokens::Token token = scanner_.scanToken();

    if (token.isError()) {
      addError(token);
      continue;
    }

    state_->addToken(token);

    if (token.getType() == tokens::TokenType::END_OF_FILE) {
      break;
    }
  }

  return state_->getTokens();
}

void Lexer::addError(const tokens::Token &token) {
  std::string error =
      "Lexical error at line " + std::to_string(token.getLocation().getLine()) +
      ", column " + std::to_string(token.getLocation().getColumn()) + ": " +
      token.getErrorMessage().value_or("Unknown error");
  errors_.push_back(error);
}

} // namespace lexer