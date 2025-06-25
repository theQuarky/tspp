/*****************************************************************************
 * File: token_scanner.h
 * Description: Main scanner that coordinates specialized scanners for TSPP
 *lexical analysis
 *****************************************************************************/
#pragma once
#include "specialized/identifier_scanner.h"
#include "specialized/number_scanner.h"
#include "specialized/operator_scanner.h"
#include "specialized/string_scanner.h"
#include <memory>

namespace lexer {

class TokenScanner {
public:
  explicit TokenScanner(std::shared_ptr<LexerState> state);

  /**
   * @brief Scan next token from input
   * @return Next token in the input stream
   */
  tokens::Token scanToken();

private:
  std::shared_ptr<LexerState> state_;
  IdentifierScanner identifierScanner_;
  NumberScanner numberScanner_;
  OperatorScanner operatorScanner_;
  StringScanner stringScanner_;

  void skipWhitespace();
  bool checkComment();
  void skipLineComment();
  void skipBlockComment();
  tokens::Token makeEndToken();
};

} // namespace lexer