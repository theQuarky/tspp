/*****************************************************************************
 * File: operator_scanner.h
 * Description: Scanner component for operators in TSPP language.
 *
 * Key Features:
 *  - Single character operator recognition
 *  - Compound operator scanning
 *  - Operator validation
 *****************************************************************************/

#pragma once
#include "lexer/scanner/base/scanner_base.h"

namespace lexer {

/**
 * @brief Scanner for operator tokens in TSPP language
 */
class OperatorScanner : public ScannerBase {
public:
  /**
   * @brief Construct operator scanner with lexer state
   * @param state Shared lexer state for scanning
   */
  explicit OperatorScanner(std::shared_ptr<LexerState> state);

  /**
   * @brief Scan next operator token from input
   * @return Token representing scanned operator
   */
  tokens::Token scan();

private:
  /**
   * @brief Map operator lexeme to corresponding token type
   * @param lexeme Operator lexeme to map
   * @return TokenType for operator or ERROR_TOKEN if invalid
   */
  tokens::TokenType getOperatorType(std::string_view lexeme);

  /**
   * @brief Check if character can start compound operator
   * @param c Character to check
   * @return true if character can be compound operator start
   */
  bool isCompoundOperator(char c) const;

  /**
   * @brief Try to scan second part of compound operator
   * @return true if compound operator successfully scanned
   */
  bool scanCompoundOperator();
};

} // namespace lexer