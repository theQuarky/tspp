#pragma once
#include "core/diagnostics/error_reporter.h"
#include "ideclaration_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "parser/visitors/parse_visitor/statement/istatement_visitor.h"
#include "tokens/stream/token_stream.h"
#include "tokens/token_type.h"
#include <iostream>
#include <ostream>

namespace visitors {

class ClassDeclarationVisitor {
public:
  ClassDeclarationVisitor(tokens::TokenStream &tokens,
                          core::ErrorReporter &errorReporter,
                          IDeclarationVisitor &declVisitor,
                          IExpressionVisitor &exprVisitor,
                          IStatementVisitor &stmtVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        declVisitor_(declVisitor), exprVisitor_(exprVisitor),
        stmtVisitor_(stmtVisitor) {}

  nodes::DeclPtr
  parseClassDecl(const std::vector<tokens::TokenType> &initialModifiers = {}) {
    auto location = tokens_.peek().getLocation();
    std::vector<tokens::TokenType> modifiers = initialModifiers;

    // Expect 'class' keyword - using lexeme check
    if (tokens_.peek().getLexeme() != "class") {
      error("Expected 'class' keyword");
      return nullptr;
    }
    tokens_.advance();

    // Parse class name
    if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
      error("Expected class name after 'class'");
      return nullptr;
    }
    auto className = tokens_.peek().getLexeme();
    tokens_.advance();

    // Parse optional generic parameters
    std::vector<nodes::TypePtr> genericParams;
    if (tokens_.peek().getLexeme() == "<") {
      tokens_.advance();
      if (!parseGenericParams(genericParams)) {
        return nullptr;
      }
    }

    // Parse optional "extends" (base class)
    nodes::TypePtr baseClass;
    if (tokens_.peek().getLexeme() == "extends") {
      tokens_.advance();
      baseClass = declVisitor_.parseType();
      if (!baseClass) {
        error("Expected base class type after 'extends'");
        return nullptr;
      }
    }

    // Parse optional "implements" list
    std::vector<nodes::TypePtr> interfaces;
    if (tokens_.peek().getLexeme() == "implements") {
      tokens_.advance();
      do {
        auto ifaceType = declVisitor_.parseType();
        if (!ifaceType) {
          error("Expected interface name after 'implements'");
          return nullptr;
        }
        interfaces.push_back(ifaceType);
      } while (match(tokens::TokenType::COMMA));
    }

    // Expect '{' to start class body
    if (tokens_.peek().getLexeme() != "{") {
      error("Expected '{' before class body");
      return nullptr;
    }
    tokens_.advance();

    // Parse class members
    std::vector<nodes::DeclPtr> members;
    int memberCount = 0;

    // Improved class body parsing loop
    while (!tokens_.isAtEnd()) {
      // Check for end of class
      if (tokens_.peek().getLexeme() == "}") {
        break;
      }

      // Try to parse a member
      auto member = parseMemberDecl();
      if (member) {
        members.push_back(std::move(member));
        memberCount++;
      } else {
        // Skip tokens until we find something that looks like a member start
        synchronize();

        // If we're now at the end of the class, break out
        if (tokens_.peek().getLexeme() == "}" || tokens_.isAtEnd()) {
          break;
        }
      }
    }

    // Consume the closing brace if present
    if (tokens_.peek().getLexeme() == "}") {
      tokens_.advance(); // Advance without error reporting
    } else {
      error("Expected '}' at end of class declaration");
    }

    // Create appropriate class node based on whether it has generic parameters
    if (!genericParams.empty()) {
      return std::make_shared<nodes::GenericClassDeclNode>(
          className, modifiers, std::move(baseClass), std::move(interfaces),
          std::move(members), std::move(genericParams), location);
    } else {
      return std::make_shared<nodes::ClassDeclNode>(
          className, modifiers, std::move(baseClass), std::move(interfaces),
          std::move(members), location);
    }
  }

  nodes::DeclPtr parseMemberDecl() {
    try {
      auto location = tokens_.peek().getLocation();

      // Check for an access modifier using lexeme
      std::vector<tokens::TokenType> modifiers;
      parseMethodModifiers(modifiers);

      tokens::TokenType accessModifier = tokens::TokenType::ERROR_TOKEN;
      std::string accessLexeme = tokens_.peek().getLexeme();
      if (accessLexeme == "public" || accessLexeme == "private" ||
          accessLexeme == "protected") {
        accessModifier = tokens_.peek().getType();
        tokens_.advance(); // consume it
      }

      // Handle different member types USING LEXEMES
      std::string tokenLexeme = tokens_.peek().getLexeme();
      if (tokenLexeme == "constructor") {
        return parseConstructor(accessModifier);
      } else if (tokenLexeme == "function") {
        return parseMethod(accessModifier, modifiers);
      } else if (tokenLexeme == "let" || tokenLexeme == "const") {
        return parseField(accessModifier);
      } else if (tokenLexeme == "get") {
        return parsePropertyGetter(accessModifier);
      } else if (tokenLexeme == "set") {
        return parsePropertySetter(accessModifier);
      } else if (tokenLexeme == "class") {
        // Handle nested class declarations
        return parseNestedClass(accessModifier);
      } else if (check(tokens::TokenType::IDENTIFIER)) {
        return parseMethod(accessModifier);
      } else {
        error("Expected class member declaration, found: " + tokenLexeme);
        return nullptr;
      }
    } catch (const std::exception &e) {
      error(std::string("Error parsing class member: ") + e.what());
      return nullptr;
    }
  }

  // Add support for nested classes
  nodes::DeclPtr parseNestedClass(tokens::TokenType accessModifier) {
    // We've already verified this is a "class" token
    tokens_.advance(); // Consume "class"

    // Use the same class parsing logic, but track that it's a nested class
    auto location = tokens_.previous().getLocation();

    if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
      error("Expected class name after 'class'");
      return nullptr;
    }
    auto className = tokens_.peek().getLexeme();
    tokens_.advance();

    // Parse optional generic parameters
    std::vector<nodes::TypePtr> genericParams;
    if (tokens_.peek().getLexeme() == "<") {
      tokens_.advance();
      if (!parseGenericParams(genericParams)) {
        return nullptr;
      }
    }

    // Parse optional "extends" (base class)
    nodes::TypePtr baseClass;
    if (tokens_.peek().getLexeme() == "extends") {
      tokens_.advance();
      baseClass = declVisitor_.parseType();
      if (!baseClass) {
        error("Expected base class type after 'extends'");
        return nullptr;
      }
    }

    // Parse optional "implements" list
    std::vector<nodes::TypePtr> interfaces;
    if (tokens_.peek().getLexeme() == "implements") {
      tokens_.advance();
      do {
        auto ifaceType = declVisitor_.parseType();
        if (!ifaceType) {
          error("Expected interface name after 'implements'");
          return nullptr;
        }
        interfaces.push_back(ifaceType);
      } while (match(tokens::TokenType::COMMA));
    }

    // Expect '{' to start class body
    if (tokens_.peek().getLexeme() != "{") {
      error("Expected '{' before nested class body");
      return nullptr;
    }
    tokens_.advance();

    // Parse nested class members
    std::vector<nodes::DeclPtr> members;

    while (!tokens_.isAtEnd()) {
      // Check for end of class
      if (tokens_.peek().getLexeme() == "}") {
        break;
      }

      // Try to parse a member
      auto member = parseMemberDecl();
      if (member) {
        members.push_back(std::move(member));
      } else {
        // Skip tokens until we find something that looks like a member start
        synchronize();

        // If we're now at the end of the class, break out
        if (tokens_.peek().getLexeme() == "}" || tokens_.isAtEnd()) {
          break;
        }
      }
    }

    // Consume the closing brace
    if (tokens_.peek().getLexeme() == "}") {
      tokens_.advance();
    } else {
      error("Expected '}' after nested class body");
    }

    // Create the appropriate class node
    std::vector<tokens::TokenType> classModifiers;

    if (!genericParams.empty()) {
      return std::make_shared<nodes::GenericClassDeclNode>(
          className, classModifiers, std::move(baseClass),
          std::move(interfaces), std::move(members), std::move(genericParams),
          location);
    } else {
      return std::make_shared<nodes::ClassDeclNode>(
          className, classModifiers, std::move(baseClass),
          std::move(interfaces), std::move(members), location);
    }
  }

  bool parseMethodModifiers(std::vector<tokens::TokenType> &modifiers) {
    while (true) {
      auto token = tokens_.peek();
      tokens::TokenType type = token.getType();
      std::string lexeme = token.getLexeme();

      // Check for function modifiers by lexeme
      if (lexeme == "#inline" || lexeme == "#virtual" || lexeme == "#unsafe" ||
          lexeme == "#simd" || lexeme.substr(0, 1) == "#") {
        modifiers.push_back(type);
        tokens_.advance();
      } else {
        break;
      }
    }
    return true;
  }

  nodes::DeclPtr parseConstructor(tokens::TokenType accessModifier) {
    auto location = tokens_.peek().getLocation();

    // Check lexeme instead of token type
    if (tokens_.peek().getLexeme() != "constructor") {
      error("Expected 'constructor' keyword");
      return nullptr;
    }
    tokens_.advance();

    // Parameter list: constructor()
    if (tokens_.peek().getLexeme() != "(") {
      error("Expected '(' after 'constructor'");
      return nullptr;
    }
    tokens_.advance();

    std::vector<nodes::ParamPtr> parameters;
    if (tokens_.peek().getLexeme() != ")") {
      if (!parseParameterList(parameters)) {
        return nullptr;
      }
    }

    if (tokens_.peek().getLexeme() != ")") {
      error("Expected ')' after constructor parameters");
      return nullptr;
    }
    tokens_.advance();

    // Parse constructor body block
    if (tokens_.peek().getLexeme() != "{") {
      error("Expected '{' before constructor body");
      return nullptr;
    }

    tokens_.advance(); // Consume the opening brace
    auto body = stmtVisitor_.parseBlock();
    if (!body) {
      return nullptr;
    }

    // Build a ConstructorDeclNode
    return std::make_shared<nodes::ConstructorDeclNode>(
        accessModifier, std::move(parameters), std::move(body), location);
  }

  // Update the function declaration to accept modifiers
  nodes::DeclPtr
  parseMethod(tokens::TokenType accessModifier,
              const std::vector<tokens::TokenType> &methodModifiers = {}) {
    auto location = tokens_.peek().getLocation();

    // Consume the 'function' keyword - using lexeme check
    if (tokens_.peek().getLexeme() != "function") {
      error("Expected 'function' keyword");
      return nullptr;
    }
    tokens_.advance();

    // Parse method name
    if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
      error("Expected method name after 'function'");
      return nullptr;
    }
    auto methodName = tokens_.peek().getLexeme();
    tokens_.advance();

    // Parse parameter list
    if (tokens_.peek().getLexeme() != "(") {
      error("Expected '(' after method name");
      return nullptr;
    }
    tokens_.advance();

    std::vector<nodes::ParamPtr> parameters;
    if (tokens_.peek().getLexeme() != ")") {
      if (!parseParameterList(parameters)) {
        return nullptr;
      }
    }

    if (tokens_.peek().getLexeme() != ")") {
      error("Expected ')' after parameters");
      return nullptr;
    }
    tokens_.advance();

    // Parse optional return type
    nodes::TypePtr returnType;
    if (tokens_.peek().getLexeme() == ":") {
      tokens_.advance();
      returnType = declVisitor_.parseType();
      if (!returnType) {
        return nullptr;
      }
    }

    // Parse optional throws clause
    std::vector<nodes::TypePtr> throwsTypes;
    if (tokens_.peek().getLexeme() == "throws") {
      tokens_.advance();
      do {
        auto thrownType = declVisitor_.parseType();
        if (!thrownType) {
          return nullptr;
        }
        throwsTypes.push_back(std::move(thrownType));
      } while (match(tokens::TokenType::COMMA));
    }

    // Parse the method body
    if (tokens_.peek().getLexeme() != "{") {
      error("Expected '{' before method body");
      return nullptr;
    }

    tokens_.advance(); // Consume the opening brace
    auto body = stmtVisitor_.parseBlock();
    if (!body) {
      return nullptr;
    }

    // Create the method node - now passing methodModifiers
    return std::make_shared<nodes::MethodDeclNode>(
        methodName, accessModifier, std::move(parameters),
        std::move(returnType), std::move(throwsTypes), methodModifiers,
        std::move(body), location);
  }

  nodes::DeclPtr parseField(tokens::TokenType accessModifier,
                            bool isConst = false) {
    auto location = tokens_.peek().getLocation();

    std::string tokenLexeme = tokens_.peek().getLexeme();
    if (tokenLexeme == "let") {
      isConst = false;
      tokens_.advance();
    } else if (tokenLexeme == "const") {
      isConst = true;
      tokens_.advance();
    } else {
      error("Expected 'let' or 'const' in field declaration");
      return nullptr;
    }

    // field name
    if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
      error("Expected field name");
      return nullptr;
    }
    auto fieldName = tokens_.peek().getLexeme();
    tokens_.advance();

    // optional type annotation: ": Type"
    nodes::TypePtr fieldType;
    if (tokens_.peek().getLexeme() == ":") {
      tokens_.advance();
      fieldType = declVisitor_.parseType();
      if (!fieldType) {
        return nullptr;
      }
    }

    // optional initializer: "= expression"
    nodes::ExpressionPtr initializer;
    if (tokens_.peek().getLexeme() == "=") {
      tokens_.advance();
      initializer = exprVisitor_.parseExpression();
      if (!initializer) {
        return nullptr;
      }
    }

    // Expect semicolon at the end
    if (tokens_.peek().getLexeme() != ";") {
      error("Expected ';' after field declaration");
      return nullptr;
    }
    tokens_.advance();

    // Build FieldDeclNode
    return std::make_shared<nodes::FieldDeclNode>(
        fieldName, accessModifier, isConst, std::move(fieldType),
        std::move(initializer), location);
  }

  nodes::DeclPtr parsePropertyGetter(tokens::TokenType accessModifier) {
    auto location = tokens_.peek().getLocation();

    // Check lexeme for "get"
    if (tokens_.peek().getLexeme() != "get") {
      error("Expected 'get' keyword");
      return nullptr;
    }
    tokens_.advance();

    // property name
    if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
      error("Expected property name after 'get'");
      return nullptr;
    }
    auto propName = tokens_.peek().getLexeme();
    tokens_.advance();

    // Optional parameter list (may not be present)
    if (tokens_.peek().getLexeme() == "(") {
      tokens_.advance();
      if (tokens_.peek().getLexeme() != ")") {
        error("Expected empty parameter list for getter");
        return nullptr;
      }
      tokens_.advance();
    }

    // optional return type: e.g.  : int
    nodes::TypePtr returnType;
    if (tokens_.peek().getLexeme() == ":") {
      tokens_.advance();
      returnType = declVisitor_.parseType();
      if (!returnType) {
        return nullptr;
      }
    }

    // Parse property body
    if (tokens_.peek().getLexeme() != "{") {
      error("Expected '{' after property getter declaration");
      return nullptr;
    }

    tokens_.advance(); // Consume the opening brace
    auto body = stmtVisitor_.parseBlock();
    if (!body) {
      return nullptr;
    }

    // Build a property-decl node in "getter" mode
    return std::make_shared<nodes::PropertyDeclNode>(
        propName, accessModifier, nodes::PropertyKind::Getter, returnType, body,
        location);
  }

  nodes::DeclPtr parsePropertySetter(tokens::TokenType accessModifier) {
    auto location = tokens_.peek().getLocation();

    // Check lexeme for "set"
    if (tokens_.peek().getLexeme() != "set") {
      error("Expected 'set' keyword");
      return nullptr;
    }
    tokens_.advance();

    // property name
    if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
      error("Expected property name after 'set'");
      return nullptr;
    }
    auto propName = tokens_.peek().getLexeme();
    tokens_.advance();

    // Parse parameter for setter (value: Type)
    if (tokens_.peek().getLexeme() != "(") {
      error("Expected '(' after property setter name");
      return nullptr;
    }
    tokens_.advance();

    // Standard parameter with parentheses
    if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
      error("Expected parameter name in setter");
      return nullptr;
    }

    auto paramName = tokens_.peek().getLexeme();
    tokens_.advance();

    nodes::TypePtr paramType;
    if (tokens_.peek().getLexeme() == ":") {
      tokens_.advance();
      paramType = declVisitor_.parseType();
      if (!paramType) {
        return nullptr;
      }
    }

    if (tokens_.peek().getLexeme() != ")") {
      error("Expected ')' after setter parameter");
      return nullptr;
    }
    tokens_.advance();

    // Parse setter body block
    if (tokens_.peek().getLexeme() != "{") {
      error("Expected '{' after setter parameter list");
      return nullptr;
    }

    tokens_.advance(); // Consume the opening brace
    auto body = stmtVisitor_.parseBlock();
    if (!body) {
      return nullptr;
    }

    // Build a property-decl node in "setter" mode
    return std::make_shared<nodes::PropertyDeclNode>(
        propName, accessModifier, nodes::PropertyKind::Setter, paramType, body,
        location);
  }

  bool parseGenericParams(std::vector<nodes::TypePtr> &outParams) {
    do {
      if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
        error("Expected generic parameter name");
        return false;
      }

      // Get parameter name
      auto paramName = tokens_.peek().getLexeme();
      auto paramLoc = tokens_.peek().getLocation();
      tokens_.advance();

      // Parse constraint if present
      std::vector<nodes::TypePtr> constraints;
      if (tokens_.peek().getLexeme() == "extends") {
        tokens_.advance();

        // Handle constraint type
        if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
          error("Expected constraint type after 'extends'");
          return false;
        }

        auto constraintName = tokens_.peek().getLexeme();
        auto constraintLoc = tokens_.peek().getLocation();
        tokens_.advance();

        // Create a constraint node - either built-in or named type
        auto constraintNode = std::make_shared<nodes::NamedTypeNode>(
            constraintName, constraintLoc);
        constraints.push_back(constraintNode);

        // Parse any intersection types with &
        while (tokens_.peek().getLexeme() == "&") {
          tokens_.advance();

          if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
            error("Expected constraint type after '&'");
            return false;
          }

          auto secondConstraintName = tokens_.peek().getLexeme();
          auto secondConstraintLoc = tokens_.peek().getLocation();
          tokens_.advance();

          auto additionalConstraint = std::make_shared<nodes::NamedTypeNode>(
              secondConstraintName, secondConstraintLoc);
          constraints.push_back(additionalConstraint);
        }
      }

      // Build the generic parameter node
      auto gpNode = std::make_shared<nodes::GenericParamNode>(
          paramName, std::move(constraints), paramLoc);
      outParams.push_back(gpNode);

      // Check for comma for additional parameters
      if (tokens_.peek().getLexeme() != ",") {
        break;
      }
      tokens_.advance(); // Consume comma

    } while (true);

    // Expect closing angle bracket
    if (tokens_.peek().getLexeme() != ">") {
      error("Expected '>' after generic parameters");
      return false;
    }
    tokens_.advance(); // Consume '>'
    return true;
  }

  bool parseParameterList(std::vector<nodes::ParamPtr> &paramsOut) {
    // loop until we see ')'
    while (tokens_.peek().getLexeme() != ")" && !tokens_.isAtEnd()) {
      auto param = parseSingleParameter();
      if (!param) {
        synchronize();
        return false;
      }
      paramsOut.push_back(std::move(param));

      if (tokens_.peek().getLexeme() != ",") {
        break;
      }
      tokens_.advance(); // Consume comma
    }
    return true;
  }

  nodes::ParamPtr parseSingleParameter() {
    auto loc = tokens_.peek().getLocation();

    bool isRef = false;
    bool isConst = false;

    // optional 'ref' or 'const'
    if (tokens_.peek().getLexeme() == "ref") {
      isRef = true;
      tokens_.advance();
    }
    if (tokens_.peek().getLexeme() == "const") {
      isConst = true;
      tokens_.advance();
    }

    // parameter name
    if (tokens_.peek().getType() != tokens::TokenType::IDENTIFIER) {
      error("Expected parameter name");
      return nullptr;
    }
    auto paramName = tokens_.peek().getLexeme();
    tokens_.advance();

    // optional type: ": Type"
    nodes::TypePtr paramType;
    if (tokens_.peek().getLexeme() == ":") {
      tokens_.advance();
      paramType = declVisitor_.parseType();
      if (!paramType) {
        return nullptr;
      }
    }

    // optional default value: "= expression"
    nodes::ExpressionPtr defaultValue;
    if (tokens_.peek().getLexeme() == "=") {
      tokens_.advance();
      defaultValue = exprVisitor_.parseExpression();
      if (!defaultValue) {
        return nullptr;
      }
    }

    return std::make_shared<nodes::ParameterNode>(
        paramName, std::move(paramType), std::move(defaultValue), isRef,
        isConst, loc);
  }

private:
  bool match(tokens::TokenType type) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    return false;
  }

  bool check(tokens::TokenType type) const {
    return !tokens_.isAtEnd() && tokens_.peek().getType() == type;
  }

  bool consume(tokens::TokenType type, const std::string &errMsg) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    error(errMsg);
    return false;
  }

  void synchronize() {
    tokens_.advance();
    while (!tokens_.isAtEnd()) {
      if (tokens_.previous().getLexeme() == ";")
        return;

      // Use lexeme checks for keywords
      std::string lexeme = tokens_.peek().getLexeme();
      if (lexeme == "class" || lexeme == "function" ||
          lexeme == "constructor" || lexeme == "let" || lexeme == "const" ||
          lexeme == "public" || lexeme == "private" || lexeme == "protected" ||
          lexeme == "get" || lexeme == "set" || lexeme == "}") {
        return;
      }
      tokens_.advance();
    }
  }

  void error(const std::string &message) {
    errorReporter_.error(tokens_.peek().getLocation(), message);
  }

  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IDeclarationVisitor &declVisitor_;
  IExpressionVisitor &exprVisitor_;
  IStatementVisitor &stmtVisitor_;
};

} // namespace visitors