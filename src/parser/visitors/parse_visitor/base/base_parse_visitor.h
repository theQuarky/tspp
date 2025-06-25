#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/interfaces/base_interface.h"
#include "parser/visitors/parse_visitor/declaration/declaration_parse_visitor.h"
#include "parser/visitors/parse_visitor/expression/expression_parse_visitor.h"
#include "parser/visitors/parse_visitor/statement/statement_parse_visitor.h"
#include "tokens/stream/token_stream.h"
#include <memory>
#include <vector>

namespace visitors {

class BaseParseVisitor : public interface::BaseInterface {
public:
  explicit BaseParseVisitor(tokens::TokenStream &tokens,
                            core::ErrorReporter &errorReporter);
  ~BaseParseVisitor() override = default;

  // Main parse entry point
  bool visitParse() override;

  // Access parsed nodes
  const std::vector<nodes::NodePtr> &getNodes() const { return nodes_; }

private:
  // Node parsing helpers
  nodes::NodePtr parseDeclaration();
  nodes::NodePtr parseStatement();

  // Utility methods
  bool isDeclarationStart() const;
  void synchronize();
  void error(const std::string &message);

  // Core resources
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  std::vector<nodes::NodePtr> nodes_;

  // Main parsers - order matters for initialization
  std::unique_ptr<ExpressionParseVisitor> expressionVisitor_;
  std::unique_ptr<StatementParseVisitor> statementVisitor_;
  std::unique_ptr<DeclarationParseVisitor> declarationVisitor_;
};
} // namespace visitors