/*****************************************************************************
 * File: token_stream.h
 * Description: Provides sequential access to a stream of tokens with lookahead
 * and matching capabilities. Manages a sequence of tokens and current position,
 * offering methods for token consumption and inspection.
 *****************************************************************************/

#pragma once
#include "../tokens.h"
#include <vector>

namespace tokens {

/**
 * @class TokenStream
 * @brief Manages sequential access to a stream of tokens with lookahead
 * capabilities
 *
 * TokenStream provides functionality for:
 * - Sequential token access and consumption
 * - Lookahead and previous token access
 * - Token type matching and pattern matching
 * - Error recovery through synchronization
 * - Position tracking and management
 */
class TokenStream {
public:
  /**
   * @brief Constructs a token stream from a vector of tokens
   * @param tokens Vector of tokens to manage (takes ownership)
   * @note Ensures an EOF token at the end of the stream
   */
  explicit TokenStream(std::vector<Token> tokens);

  /**
   * @brief Peek at current token without consuming it
   * @return Reference to current token
   * @note Returns EOF token if beyond stream end
   */
  const Token &peek() const;

  /**
   * @brief Peek at next token without consuming it
   * @return Reference to next token
   * @note Returns EOF token if beyond stream end
   */
  const Token &peekNext(int n = 1) const;

  /**
   * @brief Get previously consumed token
   * @return Reference to previous token
   * @note Returns first token if at start, EOF if beyond end
   */
  const Token &previous() const;

  /**
   * @brief Advance to next token and return previous
   * @return Token that was current before advancing
   * @note Stops at EOF
   */
  Token advance();

  /**
   * @brief Check if stream has reached end
   * @return true if at EOF or last token
   */
  bool isAtEnd() const;

  /**
   * @brief Try to match current token with expected type
   * @param type TokenType to match against
   * @return true if matched and consumed, false otherwise
   */
  bool match(TokenType type);

  /**
   * @brief Try to match current token against multiple types
   * @param types Vector of TokenTypes to match against
   * @return true if any type matched and token consumed
   */
  bool matchAny(const std::vector<TokenType> &types);

  /**
   * @brief Synchronize stream state after error
   * @note Advances until statement boundary or major declaration
   */
  void synchronize();

  /**
   * @brief Get current position in token stream
   * @return Current token index
   */
  size_t getCurrentPosition() const;

  /**
   * @brief Set position in token stream
   * @param position New position index
   * @note Clamps to valid range
   */
  void setPosition(size_t position);

  /**
   * @brief Save current position for later restoration
   * @return Current position index
   */
  size_t savePosition() const;

  /**
   * @brief Restore previously saved position
   * @param position Previously saved position
   */
  void restorePosition(size_t position);

  /**
   * @brief Check if current token matches type without consuming
   * @param type TokenType to check against
   * @return true if matches, false if not or at end
   */
  bool check(TokenType type) const;

  /*
   * return current token
   */
  Token getCurrentToken();

private:
  std::vector<Token> tokens_; ///< Storage for token sequence
  size_t current_;            ///< Current position in token stream
};

} // namespace tokens