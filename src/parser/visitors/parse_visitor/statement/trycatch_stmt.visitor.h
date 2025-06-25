// trycatch_stmt.visitor.h
#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/statement_nodes.h"
#include "parser/visitors/parse_visitor/statement/istatement_visitor.h"
#include "tokens/stream/token_stream.h"

namespace visitors {

class StatementParseVisitor;

class TryCatchStatementVisitor {
public:
  TryCatchStatementVisitor(tokens::TokenStream &tokens,
                           core::ErrorReporter &errorReporter,
                           IStatementVisitor &stmtVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        stmtVisitor_(stmtVisitor) {}

  nodes::StmtPtr parseTryStatement() {
    auto location = tokens_.previous().getLocation();

    // Parse try block
    auto tryBlock = stmtVisitor_.parseStatement();
    if (!tryBlock)
      return nullptr;

    // Parse catch clauses
    std::vector<nodes::TryStmtNode::CatchClause> catchClauses;
    while (match(tokens::TokenType::CATCH)) {
      auto catchClause = parseCatchClause();
      if (!catchClause.body) {
        return nullptr;
      }
      catchClauses.push_back(std::move(catchClause));
    }

    // Parse optional finally block (fix by checking for FINALLY token type)
    nodes::StmtPtr finallyBlock;
    if (match(tokens::TokenType::FINALLY)) { // Add FINALLY to TokenType enum
      finallyBlock = stmtVisitor_.parseStatement();
      if (!finallyBlock)
        return nullptr;
    }

    // Validate structure
    if (catchClauses.empty() && !finallyBlock) {
      error("Try statement must have at least one catch or finally clause");
      return nullptr;
    }

    return std::make_shared<nodes::TryStmtNode>(
        tryBlock, std::move(catchClauses), finallyBlock, location);
  }

private:
  nodes::TryStmtNode::CatchClause parseCatchClause() {
    nodes::TryStmtNode::CatchClause clause;

    if (!consume(tokens::TokenType::LEFT_PAREN, "Expected '(' after 'catch'")) {
      return clause;
    }

    // Parse parameter name
    if (!match(tokens::TokenType::IDENTIFIER)) {
      error("Expected catch parameter name");
      return clause;
    }
    clause.parameter = tokens_.previous().getLexeme();

    // Parse optional parameter type
    if (match(tokens::TokenType::COLON)) {
      // Note: We need type parsing capabilities here
      // For now, just expect an identifier as the type
      if (!match(tokens::TokenType::IDENTIFIER)) {
        error("Expected type after ':'");
        return clause;
      }
      auto typeName = tokens_.previous().getLexeme();
      auto typeLocation = tokens_.previous().getLocation();
      clause.parameterType =
          std::make_shared<nodes::NamedTypeNode>(typeName, typeLocation);
    }

    if (!consume(tokens::TokenType::RIGHT_PAREN,
                 "Expected ')' after catch parameter")) {
      return clause;
    }

    // Parse catch block
    clause.body = stmtVisitor_.parseStatement();
    return clause;
  }

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
  IStatementVisitor &stmtVisitor_;
  ;
};

} // namespace visitors