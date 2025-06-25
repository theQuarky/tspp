/*****************************************************************************
 * File: operator_scanner.cpp
 * Description: Implementation of operator token scanning functionality.
 *****************************************************************************/

#include "operator_scanner.h"
#include "lexer/patterns/lexer_patterns.h"
#include "tokens/token_type.h"
#include <iostream>
#include <string_view>
#include <unordered_map>

namespace lexer {

/*****************************************************************************
 * Operator Token Mapping
 *****************************************************************************/
namespace {
const std::unordered_map<std::string_view, tokens::TokenType> operatorMap = {
    // Arithmetic operators
    {"+", tokens::TokenType::PLUS},
    {"-", tokens::TokenType::MINUS},
    {"*", tokens::TokenType::STAR},
    {"/", tokens::TokenType::SLASH},
    {"%", tokens::TokenType::PERCENT},
    {"?", tokens::TokenType::QUESTION},

    // Braces and parentheses
    {"{", tokens::TokenType::LEFT_BRACE},
    {"}", tokens::TokenType::RIGHT_BRACE},
    {"(", tokens::TokenType::LEFT_PAREN},
    {")", tokens::TokenType::RIGHT_PAREN},
    {"[", tokens::TokenType::LEFT_BRACKET},
    {"]", tokens::TokenType::RIGHT_BRACKET},

    // Bitwise operators
    {"&", tokens::TokenType::AMPERSAND},
    {"|", tokens::TokenType::PIPE},
    {"^", tokens::TokenType::CARET},
    {"~", tokens::TokenType::TILDE},
    {">>", tokens::TokenType::RIGHT_SHIFT},
    {"<<", tokens::TokenType::LEFT_SHIFT},

    // Logical operators
    {"!", tokens::TokenType::EXCLAIM},
    {"&&", tokens::TokenType::AMPERSAND_AMPERSAND},
    {"||", tokens::TokenType::PIPE_PIPE},

    // Comparison operators
    {"=", tokens::TokenType::EQUALS},
    {"==", tokens::TokenType::EQUALS_EQUALS},
    {"!=", tokens::TokenType::EXCLAIM_EQUALS},
    {"<", tokens::TokenType::LESS},
    {">", tokens::TokenType::GREATER},
    {"<=", tokens::TokenType::LESS_EQUALS},
    {">=", tokens::TokenType::GREATER_EQUALS},

    // Assignment operators
    {"+=", tokens::TokenType::PLUS_EQUALS},
    {"-=", tokens::TokenType::MINUS_EQUALS},
    {"*=", tokens::TokenType::STAR_EQUALS},
    {"/=", tokens::TokenType::SLASH_EQUALS},
    {"%=", tokens::TokenType::PERCENT_EQUALS},
    {"&=", tokens::TokenType::AMPERSAND_EQUALS},
    {"|=", tokens::TokenType::PIPE_EQUALS},
    {"^=", tokens::TokenType::CARET_EQUALS},

    // Other operators
    {"++", tokens::TokenType::PLUS_PLUS},
    {"--", tokens::TokenType::MINUS_MINUS},
    {"->", tokens::TokenType::ARROW},
    {".", tokens::TokenType::DOT},
    {"@", tokens::TokenType::AT},

    // type assign operator
    {":", tokens::TokenType::COLON},
    {",", tokens::TokenType::COMMA},
    // end of the line
    {";", tokens::TokenType::SEMICOLON}};

} // namespace

/*****************************************************************************
 * Constructor Implementation
 *****************************************************************************/
OperatorScanner::OperatorScanner(std::shared_ptr<LexerState> state)
    : ScannerBase(std::move(state)) {}

/*****************************************************************************
 * Public Methods Implementation
 *****************************************************************************/
tokens::Token OperatorScanner::scan() {
  size_t start = state_->getPosition();
  char firstChar = peek();
  advance();
  // Try compound operator first (+=, -=, ++, etc)
  if (isCompoundOperator(firstChar)) {
    char nextChar = peek();
    if (!isAtEnd()) {
      // Try two-character compound operator
      std::string compound{firstChar, nextChar};
      tokens::TokenType compoundType =
          getOperatorType(std::string_view(compound));

      if (compoundType != tokens::TokenType::ERROR_TOKEN) {
        advance(); // consume second character
        return makeToken(compoundType, start, 2);
      }
    }
  }

  // Single character operator
  tokens::TokenType type = getOperatorType(std::string_view(&firstChar, 1));
  if (type != tokens::TokenType::ERROR_TOKEN) {
    return makeToken(type, start, 1);
  }

  return makeErrorToken("Invalid operator");
}

/*****************************************************************************
 * Private Helper Methods Implementation
 *****************************************************************************/
bool OperatorScanner::isCompoundOperator(char c) const {
  static const std::string compoundable = "+-*/%&|^<>=!.";
  return compoundable.find(c) != std::string::npos;
}

bool OperatorScanner::scanCompoundOperator() {
  char nextChar = peek();
  if (isAtEnd() || !LexerPatterns::canBeCompound(nextChar)) {
    return false;
  }
  advance();
  return true;
}

tokens::TokenType OperatorScanner::getOperatorType(std::string_view lexeme) {
  auto it = operatorMap.find(lexeme);
  return it != operatorMap.end() ? it->second : tokens::TokenType::ERROR_TOKEN;
}

} // namespace lexer