#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/statement_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "parser/visitors/parse_visitor/statement/istatement_visitor.h"
#include "tokens/stream/token_stream.h"
#include <iostream>
#include <ostream>

namespace visitors {

class ExpressionParseVisitor;
class StatementParseVisitor;

class BranchStatementVisitor {
public:
  BranchStatementVisitor(tokens::TokenStream &tokens,
                         core::ErrorReporter &errorReporter,
                         IExpressionVisitor &exprVisitor,
                         IStatementVisitor &stmtVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        exprVisitor_(exprVisitor), stmtVisitor_(stmtVisitor) {}

  nodes::StmtPtr parseIfStatement() {
    auto location = tokens_.previous().getLocation();

    if (!consume(tokens::TokenType::LEFT_PAREN, "Expected '(' after 'if'")) {
      return nullptr;
    }

    // Parse condition using the expression visitor
    auto condition = exprVisitor_.parseExpression();
    if (!condition) {
      return nullptr;
    }

    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after condition")) {
      return nullptr;
    }

    // Parse then branch
    auto thenBranch = stmtVisitor_.parseStatement();
    if (!thenBranch) {
      return nullptr;
    }

    // Parse optional else branch
    nodes::StmtPtr elseBranch;
    if (match(tokens::TokenType::ELSE)) {
      elseBranch = stmtVisitor_.parseStatement();
      if (!elseBranch) {
        return nullptr;
      }
    }

    return std::make_shared<nodes::IfStmtNode>(condition, thenBranch,
                                               elseBranch, location);
  }

  nodes::StmtPtr parseSwitchStatement() {
    auto location = tokens_.previous().getLocation();
    
    // Parse the opening parenthesis
    if (!consume(tokens::TokenType::LEFT_PAREN, "Expected '(' after 'switch'")) {
      return nullptr;
    }
    
    // Parse the switch expression
    auto expression = exprVisitor_.parseExpression();
    if (!expression) {
      return nullptr;
    }
    
    // Parse the closing parenthesis
    if (!consume(tokens::TokenType::RIGHT_PAREN, "Expected ')' after switch expression")) {
      return nullptr;
    }
    
    // Parse the opening brace
    if (!consume(tokens::TokenType::LEFT_BRACE, "Expected '{' after switch expression")) {
      return nullptr;
    }
    
    // Parse the case clauses
    std::vector<nodes::SwitchCase> cases;
    bool hasDefaultCase = false;
    
    while (!check(tokens::TokenType::RIGHT_BRACE) && !tokens_.isAtEnd()) {
      // Case or default clause
      if (match(tokens::TokenType::CASE) || match(tokens::TokenType::DEFAULT)) {
        bool isDefault = tokens_.previous().getType() == tokens::TokenType::DEFAULT;
        
        // Default case should only appear once
        if (isDefault && hasDefaultCase) {
          error("Cannot have more than one default clause in a switch statement");
          return nullptr;
        }
        
        if (isDefault) {
          hasDefaultCase = true;
        }
        
        // For case, parse the expression; for default, expression is nullptr
        nodes::ExpressionPtr caseExpr = nullptr;
        if (!isDefault) {
          caseExpr = exprVisitor_.parseExpression();
          if (!caseExpr) {
            return nullptr;
          }
        }
        
        // Parse the colon after case expr or default
        if (!consume(tokens::TokenType::COLON, "Expected ':' after case expression")) {
          return nullptr;
        }
        
        // Check for and skip erroneous semicolon that might be inserted after colon
        if (check(tokens::TokenType::SEMICOLON)) {
          tokens_.advance(); // Skip the semicolon
        }
        
        // Parse statements until next case, default, or closing brace
        std::vector<nodes::StmtPtr> statements;
        while (!check(tokens::TokenType::CASE) && 
               !check(tokens::TokenType::DEFAULT) && 
               !check(tokens::TokenType::RIGHT_BRACE) && 
               !tokens_.isAtEnd()) {
          auto stmt = stmtVisitor_.parseStatement();
          if (stmt) {
            statements.push_back(std::move(stmt));
          } else {
            // If statement parsing failed, try to synchronize and continue
            tokens_.advance();
          }
        }
        
        // Add the case to our list
        cases.emplace_back(isDefault, std::move(caseExpr), std::move(statements));
      } else {
        error("Expected 'case' or 'default' in switch statement");
        return nullptr;
      }
    }
    
    // Parse the closing brace
    if (!consume(tokens::TokenType::RIGHT_BRACE, "Expected '}' after switch cases")) {
      return nullptr;
    }
    
    return std::make_shared<nodes::SwitchStmtNode>(expression, std::move(cases), location);
  }

private:
  inline bool match(tokens::TokenType type) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    return false;
  }

  inline bool check(tokens::TokenType type) const {
    return !tokens_.isAtEnd() && tokens_.peek().getType() == type;
  }

  inline bool consume(tokens::TokenType type, const std::string &message) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    error(message);
    return false;
  }

  inline void error(const std::string &message) {
    errorReporter_.error(tokens_.peek().getLocation(), message);
  }

  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &exprVisitor_;
  IStatementVisitor &stmtVisitor_;
  ;
};

} // namespace visitors