/*****************************************************************************
 * File: string_scanner.h
 * Description: Scanner component for string and character literals in TSPP.
 *
 * Key Features:
 *  - String literal scanning
 *  - Character literal scanning
 *  - Escape sequence handling
 *  - Character validation
 *****************************************************************************/

#pragma once
#include "lexer/scanner/base/scanner_base.h"

namespace lexer {

/**
 * @brief Scanner for string and character literals
 */
class StringScanner : public ScannerBase {
public:
  /**
   * @brief Construct string scanner with lexer state
   * @param state Shared lexer state for scanning
   */
  explicit StringScanner(std::shared_ptr<LexerState> state);

  /**
   * @brief Scan string literal token
   * @return Token representing string literal
   */
  tokens::Token scanString();

  /**
   * @brief Scan character literal token
   * @return Token representing character literal
   */
  tokens::Token scanCharacter();

private:
  /**
   * @brief Process escape sequence in string/char literal
   * @return Valid escape sequence or empty string if invalid
   */
  std::string scanEscapeSequence();

  /**
   * @brief Check if character is valid in string/char literal
   * @param c Character to validate
   * @return true if character is valid
   */
  bool validateCharacter(char c) const;

  /**
   * @brief Validate hex/unicode escape sequence
   * @param sequence Escape sequence to validate
   * @return true if sequence is valid
   */
  bool validateEscapeSequence(const std::string &sequence) const;
};

} // namespace lexer