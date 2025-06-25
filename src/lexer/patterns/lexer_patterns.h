// lexer_patterns.h
/**
 * @file lexer_patterns.h
 * @brief Regular expression patterns and lexer utilities
 *
 * Provides regex patterns and helper functions for lexical analysis:
 * - Token pattern matching
 * - Character classification
 * - Comment handling
 * - Number format validation
 * - String and character literal processing
 */

#pragma once
#include <regex>
#include <string>
#include <unordered_set>

namespace lexer {

class LexerPatterns {
public:
  static const std::regex &getIdentifierPattern() {
    static const std::regex pattern(R"([a-zA-Z_][a-zA-Z0-9_]*)");
    return pattern;
  }

  static const std::regex &getAttributePattern() {
    // Remove the optional (?:\(([0-9]+)\))? part
    static const std::regex pattern(
        R"(#(stack|heap|static|shared|unique|weak|inline|unsafe|aligned)\b)");
    return pattern;
  }

  static const std::regex &getNumberPattern() {
    static const std::regex pattern(
        R"((?:0[xX][0-9a-fA-F]+(?:_[0-9a-fA-F]+)*)|)"
        R"((?:0[bB][01]+(?:_[01]+)*)|)"
        R"((?:[0-9]+(?:_[0-9]+)*(?:\.[0-9]+(?:_[0-9]+)*)?(?:[eE][+-]?[0-9]+(?:_[0-9]+)*)?))");
    return pattern;
  }

  static const std::regex &getStringPattern() {
    static const std::regex pattern(
        R"("(?:[^"\\]|\\['"\\nrt0]|\\x[0-9a-fA-F]{2}|\\u[0-9a-fA-F]{4})*")");
    return pattern;
  }

  static const std::regex &getCharPattern() {
    static const std::regex pattern(
        R"('(?:[^'\\]|\\['"\\nrt0]|\\x[0-9a-fA-F]{2}|\\u[0-9a-fA-F]{4})')");
    return pattern;
  }

  static const std::regex &getOperatorPattern() {
    static const std::regex pattern(
        R"(->|\.|\+\+|--|&&|\|\||==|!=|<=|>=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|<<|>>|\?\?|\?\.|\?|:|[+\-*/%&|^~!<>=@#])");
    return pattern;
  }

  // Character Classification
  static bool isOperatorStart(char c) {
    static const std::string operatorStarts =
        "+-*/%&|^~!<>=?:.@#{}()[]"; // Added {}()[]
    return operatorStarts.find(c) != std::string::npos;
  }

  static bool isDigitStart(char c) { return std::isdigit(c) || c == '.'; }

  static bool isIdentifierStart(char c) { return std::isalpha(c) || c == '_'; }

  static bool isStringStart(char c) { return c == '"'; }

  static bool isCharStart(char c) { return c == '\''; }

  static bool isWhitespace(char c) { return std::isspace(c); }

  // Attribute Validation
  static bool isValidAttribute(const std::string &attr) {
    static const std::unordered_set<std::string> validAttrs = {
        "#stack",     "#heap",    "#static",  "#shared",   "#unique",
        "#weak",      "#inline",  "#virtual", "#unsafe",   "#simd",
        "#target",    "#aligned", "#packed",  "#abstract", "#zerocast",
        "#const",     "#sizeof",  "#alignof", "#typeof",   "#asm",
        "#deprecated" // Added this
    };
    return validAttrs.count(attr) > 0;
  }

  // Token Analysis
  static bool canBeCompound(char c) {
    static const std::string compoundable = "+-*/%&|^<>=!.?:";
    return compoundable.find(c) != std::string::npos;
  }

  static size_t getMaxOperatorLength() { return 3; }

  // Number Validation
  static bool isNumberStart(const std::string &input, size_t pos);

  // Comment Detection
  static bool isLineCommentStart(const std::string &input, size_t pos) {
    return pos + 1 < input.length() && input[pos] == '/' &&
           input[pos + 1] == '/';
  }

  static bool isBlockCommentStart(const std::string &input, size_t pos) {
    return pos + 1 < input.length() && input[pos] == '/' &&
           input[pos + 1] == '*';
  }

  static bool isBlockCommentEnd(const std::string &input, size_t pos) {
    return pos + 1 < input.length() && input[pos] == '*' &&
           input[pos + 1] == '/';
  }
};

} // namespace lexer