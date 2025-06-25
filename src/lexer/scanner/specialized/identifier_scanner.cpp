/*****************************************************************************
 * File: identifier_scanner.cpp
 *
 * Description:
 *   Implementation of identifier and keyword scanning functionality.
 *   Handles lexical analysis of identifiers, keywords, and attributes.
 *****************************************************************************/

#include "identifier_scanner.h"
#include "tokens/token_type.h"
#include "tokens/tokens.h"
#include <algorithm>

namespace lexer {

namespace {
// Cache the keyword map as a static member for efficiency
const std::unordered_map<std::string, tokens::TokenType> &getKeywordMapImpl() {
  static const std::unordered_map<std::string, tokens::TokenType> keywords = {
      {"let", tokens::TokenType::LET},
      {"const", tokens::TokenType::CONST},
      {"function", tokens::TokenType::FUNCTION},
      {"class", tokens::TokenType::CLASS},
      {"constructor", tokens::TokenType::CONSTRUCTOR},
      {"interface", tokens::TokenType::INTERFACE},
      {"enum", tokens::TokenType::ENUM},
      {"typedef", tokens::TokenType::TYPEDEF},
      {"namespace", tokens::TokenType::NAMESPACE},
      {"if", tokens::TokenType::IF},
      {"else", tokens::TokenType::ELSE},
      {"for", tokens::TokenType::FOR},
      {"of", tokens::TokenType::OF},
      {"finally", tokens::TokenType::FINALLY},
      {"while", tokens::TokenType::WHILE},
      {"do", tokens::TokenType::DO},
      {"break", tokens::TokenType::BREAK},
      {"continue", tokens::TokenType::CONTINUE},
      {"return", tokens::TokenType::RETURN},
      {"true", tokens::TokenType::TRUE},
      {"false", tokens::TokenType::FALSE},
      {"null", tokens::TokenType::NULL_VALUE},
      {"undefined", tokens::TokenType::UNDEFINED},
      {"this", tokens::TokenType::THIS},
      {"void", tokens::TokenType::VOID},
      {"int", tokens::TokenType::INT},
      {"float", tokens::TokenType::FLOAT},
      {"bool", tokens::TokenType::BOOLEAN},
      {"string", tokens::TokenType::STRING},
      {"try", tokens::TokenType::TRY},
      {"catch", tokens::TokenType::CATCH},
      {"switch", tokens::TokenType::SWITCH},
      {"case", tokens::TokenType::CASE},
      {"default", tokens::TokenType::DEFAULT},
      {"extends", tokens::TokenType::EXTENDS},
      {"implements", tokens::TokenType::IMPLEMENTS},
      {"public", tokens::TokenType::PUBLIC},
      {"private", tokens::TokenType::PRIVATE},
      {"protected", tokens::TokenType::PROTECTED},
      {"new", tokens::TokenType::NEW},
      {"throw", tokens::TokenType::THROW},
      {"typeof", tokens::TokenType::TYPEOF},
      {"unsafe", tokens::TokenType::UNSAFE},
      {"aligned", tokens::TokenType::ALIGNED},
      {"ref", tokens::TokenType::REF},
      {"where", tokens::TokenType::WHERE},
      {"throws", tokens::TokenType::THROWS},
      {"get", tokens::TokenType::GET},
      {"set", tokens::TokenType::SET},
      {"#inline", tokens::TokenType::INLINE},
      {"#virtual", tokens::TokenType::VIRTUAL},
      {"#unsafe", tokens::TokenType::UNSAFE},
      {"#simd", tokens::TokenType::SIMD},
      {"#target", tokens::TokenType::TARGET},
      {"#packed", tokens::TokenType::PACKED},
      {"#abstract", tokens::TokenType::ABSTRACT},
      {"#zerocast", tokens::TokenType::ZEROCAST},
      {"cast", tokens::TokenType::CAST}};
  return keywords;
}
} // namespace

/*****************************************************************************
 * Construction
 *****************************************************************************/
IdentifierScanner::IdentifierScanner(std::shared_ptr<LexerState> state)
    : ScannerBase(std::move(state)) {}

/*****************************************************************************
 * Public Interface Implementation
 *****************************************************************************/
tokens::Token IdentifierScanner::scan() {
  size_t start = state_->getPosition();

  // Scan identifier characters
  while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
    advance();
  }

  // Directly get source data and length - avoiding intermediate string_view
  const char *data = state_->getSource().data() + start;
  size_t length = state_->getPosition() - start;

  // Check if this identifier follows @ token
  bool afterAt = false;
  if (start > 0) {
    char prevChar = state_->getSource()[start - 1];
    afterAt = (prevChar == '@');
  }

  if (afterAt) {
    // Compare directly with string_view from static memory
    std::string_view id(data, length);
    if (id == "unsafe" || id == "aligned") {
      if (id == "unsafe") {
        return makeToken(tokens::TokenType::UNSAFE, start, length);
      } else { // aligned
        // Return just the 'aligned' token
        auto token = makeToken(tokens::TokenType::ALIGNED, start, length);
        return token;
      }
    }
  }

  // For identifier type check, use a stable string_view
  std::string_view identifier(data, length);
  return makeToken(identifierType(identifier), start, length);
}

tokens::Token IdentifierScanner::scanAttribute() {
  size_t start = state_->getPosition();
  advance(); // Skip #

  size_t nameStart = state_->getPosition(); // Start after #

  // Scan attribute name
  while (!isAtEnd() && (std::isalpha(peek()) || peek() == '_')) {
    advance();
  }

  // Create a direct view of the attribute name
  const char *data = state_->getSource().data() + nameStart;
  size_t length = state_->getPosition() - nameStart;
  std::string_view attrName(data, length);

  // "#inline" | "#virtual" | "#unsafe" | "#simd"
  //                 | "#const" | "#target"
  // Map attribute to token type
  tokens::TokenType type;
  if (attrName == "stack")
    type = tokens::TokenType::STACK;
  else if (attrName == "heap")
    type = tokens::TokenType::HEAP;
  else if (attrName == "static")
    type = tokens::TokenType::STATIC;
  else if (attrName == "shared")
    type = tokens::TokenType::SHARED;
  else if (attrName == "unique")
    type = tokens::TokenType::UNIQUE;
  else if (attrName == "weak")
    type = tokens::TokenType::WEAK;
  else if (attrName == "inline")
    type = tokens::TokenType::INLINE;
  else if (attrName == "virtual") {
    type = tokens::TokenType::VIRTUAL;
  } else if (attrName == "unsafe") {
    type = tokens::TokenType::UNSAFE;
  } else if (attrName == "simd") {
    type = tokens::TokenType::SIMD;
  } else if (attrName == "const") {
    type = tokens::TokenType::CONST;
  } else if (attrName == "target") {
    type = tokens::TokenType::TARGET;
  } else if (attrName == "asm") {
    type = tokens::TokenType::ASM;
    return makeToken(type, start, state_->getPosition() - start);
  } else if (attrName == "packed") {
    type = tokens::TokenType::PACKED;
  } else if (attrName == "abstract") {
    type = tokens::TokenType::ABSTRACT;
  } else if (attrName == "aligned") {
    type = tokens::TokenType::ALIGNED;
    // Return just the 'aligned' token, let the parser handle the parentheses
    // and number
    return makeToken(type, start, state_->getPosition() - start);
  } else
    type = tokens::TokenType::ATTRIBUTE;

  // Handle aligned attribute parameters if present
  if (peek() == '(') {
    advance(); // Skip (
    while (!isAtEnd() && peek() != ')') {
      advance(); // Skip parameter
    }
    if (peek() == ')') {
      advance(); // Skip )
    }
  }

  return makeToken(type, start, state_->getPosition() - start);
}

/*****************************************************************************
 * Private Helper Methods
 *****************************************************************************/
const std::unordered_map<std::string, tokens::TokenType> &
IdentifierScanner::getKeywordMap() {
  return getKeywordMapImpl();
}

bool IdentifierScanner::validateIdentifier(std::string_view lexeme) {
  if (lexeme.empty()) {
    return false;
  }

  // First character must be letter or underscore
  if (!std::isalpha(lexeme[0]) && lexeme[0] != '_') {
    return false;
  }

  // Remaining characters must be alphanumeric or underscore
  return std::all_of(lexeme.begin() + 1, lexeme.end(),
                     [](char c) { return std::isalnum(c) || c == '_'; });
}

tokens::TokenType IdentifierScanner::identifierType(std::string_view lexeme) {
  auto &keywords = getKeywordMap();
  // Convert to string for lookup
  std::string key(lexeme);
  auto it = keywords.find(key);
  return it != keywords.end() ? it->second : tokens::TokenType::IDENTIFIER;
}

} // namespace lexer