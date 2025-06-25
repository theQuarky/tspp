#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/statement_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "parser/visitors/parse_visitor/statement/istatement_visitor.h"
#include "tokens/stream/token_stream.h"

namespace visitors {

class LoopStatementVisitor {
public:
  LoopStatementVisitor(tokens::TokenStream &tokens,
                       core::ErrorReporter &errorReporter,
                       IExpressionVisitor &exprVisitor,
                       IStatementVisitor &stmtVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        exprVisitor_(exprVisitor), stmtVisitor_(stmtVisitor) {}

  // Parse while statement: while (condition) statement
  nodes::StmtPtr parseWhileStatement() {
    auto location = tokens_.previous().getLocation();

    if (!consume(tokens::TokenType::LEFT_PAREN, "Expected '(' after 'while'")) {
      return nullptr;
    }

    auto condition = exprVisitor_.parseExpression();
    if (!condition)
      return nullptr;

    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after condition")) {
      return nullptr;
    }

    auto body = stmtVisitor_.parseStatement();
    if (!body)
      return nullptr;

    return std::make_shared<nodes::WhileStmtNode>(condition, body, location);
  }

  // Parse do-while statement: do statement while (condition);
  nodes::StmtPtr parseDoWhileStatement() {
    auto location = tokens_.previous().getLocation();

    auto body = stmtVisitor_.parseStatement();
    if (!body)
      return nullptr;

    if (!consume(tokens::TokenType::WHILE, "Expected 'while' after do block")) {
      return nullptr;
    }

    if (!consume(tokens::TokenType::LEFT_PAREN, "Expected '(' after 'while'")) {
      return nullptr;
    }

    auto condition = exprVisitor_.parseExpression();
    if (!condition)
      return nullptr;

    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after condition")) {
      return nullptr;
    }

    if (!consume(tokens::TokenType::SEMICOLON,
                 "Expected ';' after do-while statement")) {
      return nullptr;
    }

    return std::make_shared<nodes::DoWhileStmtNode>(body, condition, location);
  }

  // Parse for statement: for (init; condition; increment) statement
  // Parse for statement: for (init; condition; increment) statement
  nodes::StmtPtr parseForStatement() {
    auto location = tokens_.previous().getLocation();

    if (!consume(tokens::TokenType::LEFT_PAREN, "Expected '(' after 'for'")) {
      return nullptr;
    }

    // Handle for-of loop with 'let' or 'const'
    bool isConst = false;
    if (match(tokens::TokenType::CONST)) {
      isConst = true;
    } else if (match(tokens::TokenType::LET)) {
      isConst = false;
    } else {
      return parseTraditionalFor(location);
    }

    if (!match(tokens::TokenType::IDENTIFIER)) {
      error("Expected variable name in for loop");
      return nullptr;
    }
    auto identifier = tokens_.previous().getLexeme();

    // Optional type annotation
    nodes::TypePtr type;
    if (match(tokens::TokenType::COLON)) {
      type = exprVisitor_.parseType();
      if (!type)
        return nullptr;
    }

    // Check for 'of' keyword
    if (match(tokens::TokenType::OF)) {
      auto iterable = exprVisitor_.parseExpression();
      if (!iterable)
        return nullptr;

      if (!consume(tokens::TokenType::RIGHT_PAREN,
                   "Expected ')' after for-of clause")) {
        return nullptr;
      }

      auto body = stmtVisitor_.parseStatement();
      if (!body)
        return nullptr;

      return std::make_shared<nodes::ForOfStmtNode>(isConst, identifier,
                                                    iterable, body, location);
    }

    // Regular for loop with variable declaration
    return parseForWithDecl(location, isConst, identifier, type);
  }

private:
  // Parse rest of for-of loop after 'of' keyword
  nodes::StmtPtr parseForOfRest(const core::SourceLocation &location,
                                bool isConst, const std::string &identifier,
                                nodes::TypePtr varType) {
    auto iterable = exprVisitor_.parseExpression();
    if (!iterable)
      return nullptr;

    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after for-of clause")) {
      return nullptr;
    }

    auto body = stmtVisitor_.parseStatement();
    if (!body)
      return nullptr;

    return std::make_shared<nodes::ForOfStmtNode>(isConst, identifier, iterable,
                                                  body, location);
  }

  // Parse regular for loop with variable declaration
  nodes::StmtPtr parseForWithDecl(const core::SourceLocation &location,
                                  bool isConst, const std::string &identifier,
                                  nodes::TypePtr varType) {
    // Parse initializer
    if (!consume(tokens::TokenType::EQUALS,
                 "Expected '=' after variable name")) {
      return nullptr;
    }

    auto initialValue = exprVisitor_.parseExpression();
    if (!initialValue)
      return nullptr;

    // Create variable declaration
    auto varDecl = std::make_shared<nodes::VarDeclNode>(
        identifier, varType, initialValue, tokens::TokenType::ERROR_TOKEN,
        isConst, location);

    // Create declaration statement
    auto initializer =
        std::make_shared<nodes::DeclarationStmtNode>(varDecl, location);

    if (!consume(tokens::TokenType::SEMICOLON,
                 "Expected ';' after for loop initializer")) {
      return nullptr;
    }

    return parseForRest(location, initializer);
  }

  // Parse traditional for loop (no declaration)
  nodes::StmtPtr parseTraditionalFor(const core::SourceLocation &location) {
    nodes::StmtPtr initializer;
    if (!match(tokens::TokenType::SEMICOLON)) {
      auto expr = exprVisitor_.parseExpression();
      if (!expr)
        return nullptr;

      if (!consume(tokens::TokenType::SEMICOLON,
                   "Expected ';' after for loop initializer")) {
        return nullptr;
      }

      initializer = std::make_shared<nodes::ExpressionStmtNode>(expr, location);
    }

    return parseForRest(location, initializer);
  }

  // Parse the rest of a for loop (condition and increment)
  nodes::StmtPtr parseForRest(const core::SourceLocation &location,
                              nodes::StmtPtr initializer) {
    // Parse condition
    nodes::ExpressionPtr condition;
    if (!match(tokens::TokenType::SEMICOLON)) {
      condition = exprVisitor_.parseExpression();
      if (!condition)
        return nullptr;

      if (!consume(tokens::TokenType::SEMICOLON,
                   "Expected ';' after for loop condition")) {
        return nullptr;
      }
    }

    // Parse increment
    nodes::ExpressionPtr increment;
    if (!check(tokens::TokenType::RIGHT_PAREN)) {
      increment = exprVisitor_.parseExpression();
      if (!increment)
        return nullptr;
    }

    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after for clauses")) {
      return nullptr;
    }

    auto body = stmtVisitor_.parseStatement();
    if (!body)
      return nullptr;

    return std::make_shared<nodes::ForStmtNode>(initializer, condition,
                                                increment, body, location);
  }

  // Utility methods
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

  bool consume(tokens::TokenType type, const std::string &message) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    error(message);
    return false;
  }

  void error(const std::string &message) {
    errorReporter_.error(tokens_.peek().getLocation(), message);
  }

  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &exprVisitor_;
  IStatementVisitor &stmtVisitor_;
};

} // namespace visitors