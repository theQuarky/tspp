/*****************************************************************************
 * File: lexer.h
 * Description: Main lexer interface for TSPP language
 *****************************************************************************/

#pragma once
#include "scanner/token_scanner.h"
#include "state/lexer_state.h"
#include <memory>
#include <string>
#include <vector>

namespace lexer {

class Lexer {
public:
  explicit Lexer(std::string source, std::string filename = "");

  // Process source and return all tokens
  std::vector<tokens::Token> tokenize();

  // Get any lexical errors that occurred
  const std::vector<std::string> &getErrors() const { return errors_; }

  // Check if lexical analysis encountered errors
  bool hasErrors() const { return !errors_.empty(); }

private:
  std::shared_ptr<LexerState> state_;
  TokenScanner scanner_;
  std::vector<std::string> errors_;

  void addError(const tokens::Token &token);
};

} // namespace lexer