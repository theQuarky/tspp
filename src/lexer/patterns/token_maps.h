// token_map_utils.h
/**
 * @file token_map_utils.h
 * @brief Utility functions for token manipulation and validation
 *
 * This file provides comprehensive utilities for:
 * - Token sequence validation based on grammar rules
 * - Type system compatibility checking
 * - Modifier and attribute validation
 * - Operator classification
 * - Context-aware token analysis
 */

#pragma once
#include "tokens/token_type.h"
#include <string>
#include <unordered_set>

namespace lexer {

class TokenMapUtils {
public:
  // Operator Classification
  static bool isUnaryOperator(const std::string &op, bool prefix = true) {
    static const std::unordered_set<std::string> prefixOps = {
        "+", "-", "!", "~", "*", "&", "++", "--", "@"};
    static const std::unordered_set<std::string> postfixOps = {"++", "--"};
    return prefix ? prefixOps.count(op) > 0 : postfixOps.count(op) > 0;
  }

  static bool isBinaryOperator(const std::string &op) {
    static const std::unordered_set<std::string> binaryOps = {
        "+",  "-",  "*", "/", "%", "==", "!=", "<", ">",  "<=", ">=",
        "&&", "||", "&", "|", "^", "<<", ">>", ".", "->", "@"};
    return binaryOps.count(op) > 0;
  }

  // Type System
  static bool isTypeModifier(tokens::TokenType type) {
    static const std::unordered_set<tokens::TokenType> modifiers = {
        tokens::TokenType::SHARED, tokens::TokenType::UNIQUE,
        tokens::TokenType::WEAK, tokens::TokenType::REF};
    return modifiers.count(type) > 0;
  }

  static bool isStorageModifier(tokens::TokenType type) {
    static const std::unordered_set<tokens::TokenType> modifiers = {
        tokens::TokenType::STACK, tokens::TokenType::HEAP,
        tokens::TokenType::STATIC};
    return modifiers.count(type) > 0;
  }

  static bool isPrimitiveType(tokens::TokenType type) {
    static const std::unordered_set<tokens::TokenType> primitives = {
        tokens::TokenType::VOID, tokens::TokenType::INT,
        tokens::TokenType::FLOAT, tokens::TokenType::BOOLEAN,
        tokens::TokenType::STRING};
    return primitives.count(type) > 0;
  }

  // Function & Class Modifiers
  static bool isFunctionModifier(tokens::TokenType type) {
    static const std::unordered_set<tokens::TokenType> modifiers = {
        tokens::TokenType::INLINE, tokens::TokenType::VIRTUAL,
        tokens::TokenType::UNSAFE, tokens::TokenType::SIMD,
        tokens::TokenType::TARGET};
    return modifiers.count(type) > 0;
  }

  static bool isClassModifier(tokens::TokenType type) {
    static const std::unordered_set<tokens::TokenType> modifiers = {
        tokens::TokenType::ALIGNED, tokens::TokenType::PACKED,
        tokens::TokenType::ABSTRACT, tokens::TokenType::ZEROCAST};
    return modifiers.count(type) > 0;
  }

  // Token Classification
  static bool isLiteral(tokens::TokenType type) {
    return type >= tokens::TokenType::LITERAL_BEGIN &&
           type <= tokens::TokenType::LITERAL_END;
  }

  static bool isSpecialKeyword(tokens::TokenType type) {
    return type >= tokens::TokenType::COMPILE_BEGIN &&
           type <= tokens::TokenType::COMPILE_END;
  }

  static bool requiresParentheses(tokens::TokenType type) {
    static const std::unordered_set<tokens::TokenType> needsParams = {
        tokens::TokenType::ALIGNED, tokens::TokenType::TARGET};
    return needsParams.count(type) > 0;
  }

  // Sequence Validation
  static bool isValidTokenSequence(tokens::TokenType prev,
                                   tokens::TokenType current) {
    if (prev == tokens::TokenType::END_OF_FILE ||
        current == tokens::TokenType::END_OF_FILE) {
      return false;
    }

    if (isDeclaration(prev))
      return canFollowDeclaration(current);
    if (isType(prev))
      return canFollowType(current);
    if (isOperator(prev))
      return canFollowOperator(prev, current);
    if (isAccessModifier(prev))
      return canFollowAccessModifier(current);
    if (isStorageModifier(prev))
      return canFollowStorageModifier(current);
    if (isFunctionModifier(prev))
      return canFollowFunctionModifier(current);
    if (isClassModifier(prev))
      return canFollowClassModifier(current);

    return true;
  }

  // Type Compatibility
  static bool isModifierCompatibleWithType(tokens::TokenType modifier,
                                           tokens::TokenType type) {
    if (isSmartPointerType(modifier)) {
      return !isPrimitiveType(type) && type != tokens::TokenType::VOID;
    }

    if (isStorageModifier(modifier)) {
      return type != tokens::TokenType::VOID && !isReferenceType(type);
    }

    if (modifier == tokens::TokenType::REF) {
      return !isPrimitiveType(type) && type != tokens::TokenType::VOID;
    }

    if (modifier == tokens::TokenType::ALIGNED) {
      return !isPrimitiveType(type);
    }

    if (modifier == tokens::TokenType::UNSAFE) {
      return isPointerType(type);
    }

    return true;
  }

  static std::string getTokenCategory(tokens::TokenType type) {
    // We need to add checks for delimiters
    if (isDelimiter(type)) {
      return "Delimiter";
    }
    // Storage modifiers should come before typeModifier check
    else if (isMemoryManagement(type)) {
      return "StorageModifier";
    }
    // Rest of the existing checks
    else if (isPrimitiveType(type)) {
      return "PrimitiveType";
    } else if (isTypeModifier(type)) {
      return "TypeModifier";
    } else if (isFunctionModifier(type)) {
      return "FunctionModifier";
    } else if (isClassModifier(type)) {
      return "ClassModifier";
    } else if (isLiteral(type)) {
      return "Literal";
    } else if (isSpecialKeyword(type)) {
      return "SpecialKeyword";
    } else if (isOperator(type)) {
      return "Operator";
    } else if (isAccessModifier(type)) {
      return "AccessModifier";
    } else if (isDeclaration(type)) {
      return "Declaration";
    } else if (isType(type)) {
      return "Type";
    } else if (isGeneric(type)) {
      return "Generic";
    }

    return "Unknown";
  }

private:
  static bool isDeclaration(tokens::TokenType type) {
    return type >= tokens::TokenType::DECL_BEGIN &&
           type <= tokens::TokenType::DECL_END;
  }

  static bool isType(tokens::TokenType type) {
    return type >= tokens::TokenType::TYPE_BEGIN &&
           type <= tokens::TokenType::TYPE_END;
  }
  static bool isGeneric(tokens::TokenType type) {
    return type >= tokens::TokenType::GENERIC_BEGIN &&
           type <= tokens::TokenType::GENERIC_END;
  }
  static bool isAccessModifier(tokens::TokenType type) {
    return type >= tokens::TokenType::ACCESS_BEGIN &&
           type <= tokens::TokenType::ACCESS_END;
  }

  static bool isOperator(tokens::TokenType type) {
    return type >= tokens::TokenType::OPERATOR_BEGIN &&
           type <= tokens::TokenType::OPERATOR_END;
  }

  static bool canFollowDeclaration(tokens::TokenType type);
  static bool canFollowType(tokens::TokenType type);
  static bool canFollowOperator(tokens::TokenType prev,
                                tokens::TokenType current);
  static bool canFollowAccessModifier(tokens::TokenType type);
  static bool canFollowStorageModifier(tokens::TokenType type);
  static bool canFollowFunctionModifier(tokens::TokenType type);
  static bool canFollowClassModifier(tokens::TokenType type);

  static bool isSmartPointerType(tokens::TokenType type);
  static bool isReferenceType(tokens::TokenType type);
  static bool isPointerType(tokens::TokenType type);
};

} // namespace lexer
