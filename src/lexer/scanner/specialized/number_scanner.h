/*****************************************************************************
 * File: number_scanner.h
 *
 * Description:
 *   Scanner component for handling numeric literals in the TSPP language.
 *   Supports integer, floating-point, hexadecimal, and binary literals.
 *****************************************************************************/

#pragma once
#include "lexer/scanner/base/scanner_base.h"

namespace lexer {

class NumberScanner : public ScannerBase {
public:
  explicit NumberScanner(std::shared_ptr<LexerState> state);

  /**
   * @brief Scan a numeric token
   * @return Token representing the scanned number
   */
  tokens::Token scan();

private:
  bool validateNumber(std::string_view lexeme);
  bool scanDigits();       // Scans sequence of digits
  bool scanHexDigits();    // Scans sequence of hex digits
  bool scanBinaryDigits(); // Scans sequence of binary digits
  bool scanExponent();     // Scans exponent part of floating point
  bool scanFraction();     // Scans fractional part of floating point
};

} // namespace lexer