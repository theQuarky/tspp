#pragma once
#include "branch_stmt_visitor.h"
#include "flowctr_stmt_visitor.h"
#include "loop_stmt_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/nodes/statement_nodes.h"
#include "parser/visitors/parse_visitor/declaration/ideclaration_visitor.h"
#include "parser/visitors/parse_visitor/statement/istatement_visitor.h"
#include "trycatch_stmt.visitor.h"
#include <cassert>
#include <iostream>
#include <iterator>
#include <ostream>

namespace visitors {

class StatementParseVisitor : public IStatementVisitor {
public:
  StatementParseVisitor(tokens::TokenStream &tokens,
                        core::ErrorReporter &errorReporter,
                        IExpressionVisitor &exprVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        exprVisitor_(exprVisitor),
        branchVisitor_(tokens, errorReporter, exprVisitor, *this),
        loopVisitor_(tokens, errorReporter, exprVisitor, *this),
        flowVisitor_(tokens, errorReporter, exprVisitor),
        tryVisitor_(tokens, errorReporter, *this) {}

  void setDeclarationVisitor(IDeclarationVisitor *declVisitor) {
    declVisitor_ = declVisitor;
  }

  nodes::StmtPtr parseStatement() override {
    try {
      // Check for labeled statements
      if (tokens_.peek().getType() == tokens::TokenType::IDENTIFIER &&
          tokens_.peekNext().getLexeme() == ":") {
        // This is a labeled statement
        auto label = tokens_.peek().getLexeme();
        auto location = tokens_.peek().getLocation();

        // Consume the identifier and colon
        tokens_.advance(); // Consume identifier
        tokens_.advance(); // Consume colon

        // Parse the statement that follows the label
        auto statement = parseStatement();
        if (!statement)
          return nullptr;

        // Create a labeled statement node
        return std::make_shared<nodes::LabeledStatementNode>(label, statement,
                                                             location);
      }

      // Special handling for return statements
      if (tokens_.peek().getLexeme() == "return") {
        tokens_.advance(); // Consume "return"
        auto location = tokens_.previous().getLocation();
        nodes::ExpressionPtr value;

        // Check for value after return
        if (tokens_.peek().getLexeme() != ";") {
          value = exprVisitor_.parseExpression();
          if (!value)
            return nullptr;
        }

        if (tokens_.peek().getLexeme() != ";") {
          error("Expected ';' after return statement");
          return nullptr;
        }
        tokens_.advance(); // Consume semicolon

        return std::make_shared<nodes::ReturnStmtNode>(value, location);
      }

      // Check for declarations
      if (isDeclarationStart()) {
        return parseDeclarationStatement();
      }

      // Check for other statement types
      if (tokens_.peek().getLexeme() == "{") {
        tokens_.advance(); // Consume '{'
        return parseBlock();
      }

      if (tokens_.peek().getLexeme() == "if") {
        tokens_.advance();
        return branchVisitor_.parseIfStatement();
      }

      if (tokens_.peek().getLexeme() == "switch") {
        tokens_.advance();
        return branchVisitor_.parseSwitchStatement();
      }

      if (tokens_.peek().getLexeme() == "while") {
        tokens_.advance();
        return loopVisitor_.parseWhileStatement();
      }

      if (tokens_.peek().getLexeme() == "do") {
        tokens_.advance();
        return loopVisitor_.parseDoWhileStatement();
      }

      if (tokens_.peek().getLexeme() == "for") {
        tokens_.advance();
        return loopVisitor_.parseForStatement();
      }

      if (tokens_.peek().getLexeme() == "try") {
        tokens_.advance();
        return tryVisitor_.parseTryStatement();
      }

      if (tokens_.peek().getLexeme() == "break") {
        tokens_.advance();
        return flowVisitor_.parseBreak();
      }

      if (tokens_.peek().getLexeme() == "continue") {
        tokens_.advance();
        return flowVisitor_.parseContinue();
      }

      if (tokens_.peek().getLexeme() == "throw") {
        tokens_.advance();
        return flowVisitor_.parseThrow();
      }

      return parseExpressionStatement();
    } catch (const std::exception &e) {
      error(std::string("Error parsing statement: ") + e.what());
      synchronize();
      return nullptr;
    }
  }

  nodes::BlockPtr parseBlock() override {
    auto location = tokens_.previous().getLocation();
    std::vector<nodes::StmtPtr> statements;

    while (!tokens_.isAtEnd()) {

      // Check for end of block
      if (tokens_.peek().getLexeme() == "}") {
        break;
      }

      // Regular statement parsing
      if (auto stmt = parseStatement()) {
        statements.push_back(std::move(stmt));
      } else {
        synchronize();

        // Check if we've recovered to a block end
        if (tokens_.peek().getLexeme() == "}" || tokens_.isAtEnd()) {
          break;
        }
      }
    }

    // Check for closing brace
    if (tokens_.peek().getLexeme() != "}") {
      error("Expected '}' after block");
      return nullptr;
    }

    // Advance past the closing brace
    tokens_.advance();

    return std::make_shared<nodes::BlockNode>(std::move(statements), location);
  }

private:
  nodes::StmtPtr parseDeclarationStatement() {
    // Let the declaration visitor handle the declaration
    if (auto decl = declVisitor_->parseDeclaration()) {
      // Wrap it in a statement node
      return std::make_shared<nodes::DeclarationStmtNode>(std::move(decl),
                                                          decl->getLocation());
    }
    return nullptr;
  }

  bool isDeclarationStart() const {
    std::string lexeme = tokens_.peek().getLexeme();
    // Check for class-related declarations using lexemes
    return lexeme == "let" || lexeme == "const" || lexeme == "function" ||
           lexeme == "class" || lexeme == "constructor" || lexeme == "public" ||
           lexeme == "private" || lexeme == "protected" || lexeme == "stack" ||
           lexeme == "heap" || lexeme == "static" || lexeme == "aligned" ||
           lexeme == "packed" || lexeme == "abstract";
  }

  nodes::StmtPtr parseExpressionStatement();
  nodes::StmtPtr parseAssemblyStatement();

  // Utility methods
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

  void synchronize() {
    tokens_.advance();

    while (!tokens_.isAtEnd()) {
      if (tokens_.previous().getLexeme() == ";") {
        return;
      }

      // Use lexemes for keyword checks
      std::string lexeme = tokens_.peek().getLexeme();
      if (lexeme == "class" || lexeme == "function" || lexeme == "let" ||
          lexeme == "const" || lexeme == "if" || lexeme == "while" ||
          lexeme == "return" || lexeme == "}" || lexeme == "{") {
        return;
      }
      tokens_.advance();
    }
  }

  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  IExpressionVisitor &exprVisitor_;
  IDeclarationVisitor *declVisitor_ = nullptr;

  BranchStatementVisitor branchVisitor_;
  LoopStatementVisitor loopVisitor_;
  FlowControlVisitor flowVisitor_;
  TryCatchStatementVisitor tryVisitor_;
};

} // namespace visitors