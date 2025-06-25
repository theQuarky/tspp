// statement_parse_visitor.cpp - Only including the methods implemented in this
// file
#include "statement_parse_visitor.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/nodes/statement_nodes.h"

namespace visitors {

nodes::StmtPtr StatementParseVisitor::parseExpressionStatement() {
  auto location = tokens_.peek().getLocation();

  // Check for assembly statements
  if (tokens_.peek().getLexeme() == "#asm") {
    return parseAssemblyStatement();
  }

  // Check for labeled statements
  if (tokens_.peek().getType() == tokens::TokenType::IDENTIFIER &&
      tokens_.peekNext().getLexeme() == ":") {
    auto location = tokens_.peek().getLocation();
    auto label = tokens_.advance().getLexeme();

    // Consume the colon
    tokens_.advance();

    // Parse the statement after the label
    auto statement = parseStatement();
    if (!statement)
      return nullptr;

    return std::make_shared<nodes::LabeledStatementNode>(
        label, std::move(statement), location);
  }

  auto expr = exprVisitor_.parseExpression();
  if (!expr)
    return nullptr;

  if (tokens_.peek().getLexeme() != ";") {
    error("Expected ';' after expression");
    return nullptr;
  }
  tokens_.advance();

  return std::make_shared<nodes::ExpressionStmtNode>(expr, location);
}

nodes::StmtPtr StatementParseVisitor::parseAssemblyStatement() {
  auto location = tokens_.previous().getLocation();
  tokens_.advance();
  if (tokens_.peek().getLexeme() != "(") {
    error("Expected '(' after '#asm'");
    return nullptr;
  }
  tokens_.advance();

  if (tokens_.peek().getType() != tokens::TokenType::STRING_LITERAL) {
    error("Expected string literal containing assembly code");
    return nullptr;
  }
  std::string asmCode = tokens_.peek().getLexeme();
  tokens_.advance();

  std::vector<std::string> constraints;

  while (tokens_.peek().getLexeme() == ",") {
    tokens_.advance();
    if (tokens_.peek().getType() != tokens::TokenType::STRING_LITERAL) {
      error("Expected constraint string");
      return nullptr;
    }
    constraints.push_back(tokens_.peek().getLexeme());
    tokens_.advance();
  }

  if (tokens_.peek().getLexeme() != ")") {
    error("Expected ')' after assembly code");
    return nullptr;
  }
  tokens_.advance();

  if (tokens_.peek().getLexeme() != ";") {
    error("Expected ';' after assembly statement");
    return nullptr;
  }
  tokens_.advance();

  return std::make_shared<nodes::AssemblyStmtNode>(
      asmCode, std::move(constraints), location);
}

} // namespace visitors