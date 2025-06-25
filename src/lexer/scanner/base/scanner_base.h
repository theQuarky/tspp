/*****************************************************************************
 * File: scanner_base.h
 *
 * Description:
 *   Base scanner class providing core functionality for lexical analysis.
 *   Part of the TSPP compiler's lexical analysis system.
 *
 * Key Features:
 *   - Character inspection and manipulation
 *   - Token creation utilities
 *   - Error handling and reporting
 *   - Shared lexer state management
 *
 * Dependencies:
 *   - lexer_state.h: Lexer state tracking
 *   - tokens.h: Token definitions
 *   - common_types.h: Common type definitions
 *
 * Usage:
 *   Inherit from this class to create specialized scanners for different
 *   token types (identifiers, numbers, operators, etc.)
 *****************************************************************************/
#pragma once
#include "lexer/state/lexer_state.h"
#include "tokens/tokens.h"
#include <cstddef>
#include <memory>

namespace lexer {

/**
 * @class ScannerBase
 * @brief Base class for all specialized token scanners
 *
 * Provides common functionality for scanning and token creation:
 * - Character inspection and manipulation
 * - Token creation utilities
 * - Error handling
 * - State management
 */
class ScannerBase {
protected:
  std::shared_ptr<LexerState> state_;
  ScannerBase(std::shared_ptr<LexerState> state);

  // Utility methods for all scanners
  bool match(char expected);
  char peek() const;
  char peekNext(int n = 1) const;
  bool isAtEnd() const;
  void advance();

  // Token creation helpers
  tokens::Token makeToken(tokens::TokenType type, size_t start, size_t length);
  tokens::Token makeErrorToken(const std::string &message);

public:
  virtual ~ScannerBase() = default;
};

} // namespace lexer