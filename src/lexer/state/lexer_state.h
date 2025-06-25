/*****************************************************************************
 * File: lexer_state.h
 * Description: Maintains lexer state during tokenization process.
 * Manages source code position, line/column tracking, and token collection.
 *****************************************************************************/

#pragma once
#include "../../tokens/tokens.h"
#include <cassert>
#include <string>
#include <vector>

namespace lexer {

/**
 * @class LexerState
 * @brief Manages lexer state during tokenization process
 *
 * Provides functionality for:
 * - Source code navigation and character inspection
 * - Position and line/column tracking
 * - Token collection and management
 * - Error location reporting
 */
class LexerState {
public:
  /**
   * @brief Constructs lexer state with source code
   * @param source Source code to tokenize
   * @param fileName Source file name for error reporting
   */
  LexerState(std::string source, std::string fileName)
      : source_(std::move(source)), fileName_(std::move(fileName)),
        position_(0), line_(1), column_(1), tokens_() {}

  /*****************************************************************************
   * Source Inspection Methods
   *****************************************************************************/

  /**
   * @brief Get remaining source from current position
   * @return View of remaining unprocessed source
   */
  std::string_view getCurrentLexeme() const;

  /**
   * @brief Get current character under cursor
   * @return Current character or '\0' if at end
   */
  char getCurrentChar() const;

  /**
   * @brief Look ahead in source without advancing
   * @param n Number of characters to look ahead (negative returns current char)
   * @return Character at position + n or '\0' if out of bounds
   */
  char peekNext(int n = 0) const;

  /**
   * @brief Try to match expected character
   * @param expected Character to match
   * @return true if matched and advanced, false otherwise
   */
  bool match(char expected);

  /**
   * @brief Get current line content for error reporting
   * @return View of current line in source
   */
  std::string_view getLineContent() const;

  /*****************************************************************************
   * State Management Methods
   *****************************************************************************/

  /**
   * @brief Advance cursor by specified amount
   * @param count Number of positions to advance
   * @note Handles tabs for column tracking
   */
  void advance(size_t count = 1);

  /**
   * @brief Handle line break and automatic semicolon insertion
   */
  void newLine();

  /**
   * @brief Add token to collection
   * @param token Token to add (moved into collection)
   */
  void addToken(tokens::Token token);

  /**
   * @brief Reset lexer state to initial conditions
   */
  void reset();

  /*****************************************************************************
   * State Query Methods
   *****************************************************************************/

  /**
   * @brief Check if reached end of source
   */
  bool isAtEnd() const { return position_ > source_.length(); }

  /**
   * @brief Check if any tokens collected
   */
  bool hasTokens() const { return !tokens_.empty(); }

  /**
   * @brief Get last collected token
   * @return Reference to last token
   * @pre hasTokens() must be true
   */
  const tokens::Token &getLastToken() const;

  // Basic accessors
  const std::string &getSource() const { return source_; }
  const std::string &getFileName() const { return fileName_; }
  size_t getPosition() const { return position_; }
  unsigned int getLine() const { return line_; }
  unsigned int getColumn() const { return column_; }
  const std::vector<tokens::Token> &getTokens() const { return tokens_; }

private:
  std::string source_;                ///< Source code being tokenized
  std::string fileName_;              ///< Source file name for errors
  size_t position_;                   ///< Current position in source
  unsigned int line_;                 ///< Current line number (1-based)
  unsigned int column_;               ///< Current column number (1-based)
  std::vector<tokens::Token> tokens_; ///< Collected tokens
};

} // namespace lexer