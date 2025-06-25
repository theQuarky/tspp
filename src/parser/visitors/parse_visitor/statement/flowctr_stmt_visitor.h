#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/nodes/statement_nodes.h"
#include "parser/visitors/parse_visitor/expression/iexpression_visitor.h"
#include "tokens/stream/token_stream.h"

namespace visitors {

class ExpressionParseVisitor;

class FlowControlVisitor {
public:
  FlowControlVisitor(tokens::TokenStream &tokens,
                     core::ErrorReporter &errorReporter,
                     IExpressionVisitor &exprVisitor)
      : tokens_(tokens), errorReporter_(errorReporter),
        exprVisitor_(exprVisitor) {}

  nodes::StmtPtr parseReturn() {
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

  nodes::StmtPtr parseBreak() {
    auto location = tokens_.previous().getLocation();
    std::string label;

    // Optional label
    if (tokens_.peek().getType() == tokens::TokenType::IDENTIFIER) {
      label = tokens_.peek().getLexeme();
      tokens_.advance();
    }

    if (tokens_.peek().getLexeme() != ";") {
      error("Expected ';' after break statement");
      return nullptr;
    }
    tokens_.advance();

    return std::make_shared<nodes::BreakStmtNode>(std::move(label), location);
  }

  nodes::StmtPtr parseContinue() {
    auto location = tokens_.previous().getLocation();
    std::string label;

    // Optional label
    if (tokens_.peek().getType() == tokens::TokenType::IDENTIFIER) {
      label = tokens_.peek().getLexeme();
      tokens_.advance();
    }

    if (tokens_.peek().getLexeme() != ";") {
      error("Expected ';' after continue statement");
      return nullptr;
    }
    tokens_.advance();

    return std::make_shared<nodes::ContinueStmtNode>(std::move(label),
                                                     location);
  }

  nodes::StmtPtr parseThrow() {
    auto location = tokens_.previous().getLocation();

    auto value = exprVisitor_.parseExpression();
    if (!value)
      return nullptr;

    if (tokens_.peek().getLexeme() != ";") {
      error("Expected ';' after throw statement");
      return nullptr;
    }
    tokens_.advance();

    return std::make_shared<nodes::ThrowStmtNode>(value, location);
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
};

} // namespace visitors