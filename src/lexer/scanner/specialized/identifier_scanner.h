/*****************************************************************************
 * File: identifier_scanner.h
 *
 * Description:
 *   Scanner component for handling identifiers and keywords in the TSPP
 *language. Handles scanning of identifiers, keywords, and attribute names.
 *
 * Features:
 *   - Identifier validation
 *   - Keyword recognition
 *   - Attribute scanning
 *   - Error reporting for invalid identifiers
 *****************************************************************************/

#pragma once
#include "lexer/scanner/base/scanner_base.h"
#include "tokens/token_type.h"
#include "tokens/tokens.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace lexer {

class IdentifierScanner : public ScannerBase {
public:
  explicit IdentifierScanner(std::shared_ptr<LexerState> state);

  /**
   * @brief Scan an identifier or keyword token
   * @return Token representing identifier or keyword
   */
  tokens::Token scan();

  /**
   * @brief Scan an attribute token (starts with #)
   * @return Token representing the attribute
   */
  tokens::Token scanAttribute();

private:
  static const std::unordered_map<std::string, tokens::TokenType> &
  getKeywordMap();
  bool validateIdentifier(std::string_view lexeme);
  tokens::TokenType identifierType(std::string_view lexeme);
};

} // namespace lexer