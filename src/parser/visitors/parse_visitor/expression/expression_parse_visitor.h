#pragma once
#include "binary_visitor.h"
#include "call_visitor.h"
#include "cast_visitor.h"
#include "iexpression_visitor.h"
#include "parser/nodes/expression_nodes.h"
#include "parser/nodes/type_nodes.h"
#include "parser/visitors/parse_visitor/declaration/ideclaration_visitor.h"
#include "parser/visitors/parse_visitor/statement/istatement_visitor.h"
#include "primary_visitor.h"
#include "unary_visitor.h"
#include <cassert>

namespace visitors {
class ExpressionParseVisitor : public IExpressionVisitor {
public:
  ExpressionParseVisitor(tokens::TokenStream &tokens,
                         core::ErrorReporter &errorReporter,
                         IDeclarationVisitor *declVisitor = nullptr,
                         IStatementVisitor *stmtVisitor = nullptr);

  // Add setter methods (similar to StatementParseVisitor)
  void setDeclarationVisitor(IDeclarationVisitor *declVisitor) {
    declVisitor_ = declVisitor;
  }

  void setStatementVisitor(IStatementVisitor *stmtVisitor) {
    stmtVisitor_ = stmtVisitor;
  }
  // Main public interface
  nodes::ExpressionPtr parseExpression() override;
  nodes::ExpressionPtr parsePrimary() override;
  nodes::ExpressionPtr parseUnary() override;
  nodes::TypePtr parseType() override;
  nodes::ExpressionPtr parseNewExpression() override;
  nodes::ExpressionPtr parseFunctionExpression() override;
  nodes::ExpressionPtr parseAdditive();
  nodes::ExpressionPtr parseAssignment();
  nodes::ExpressionPtr parseMultiplicative();
  nodes::ExpressionPtr parseComparison();
  nodes::ParamPtr parseParameter();

  // Friend declarations for sub-visitors
  friend class BinaryExpressionVisitor;
  friend class UnaryExpressionVisitor;
  friend class PrimaryExpressionVisitor;
  friend class CallExpressionVisitor;
  friend class CastExpressionVisitor;

private:
  // Utility methods
  bool match(tokens::TokenType type);
  bool check(tokens::TokenType type) const;
  void error(const std::string &message);
  inline bool consume(tokens::TokenType type, const std::string &message) {
    if (check(type)) {
      tokens_.advance();
      return true;
    }
    error(message);
    return false;
  }
  // Member variables
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;

  IDeclarationVisitor *declVisitor_ = nullptr;
  IStatementVisitor *stmtVisitor_ = nullptr;

  // Sub-visitors
  BinaryExpressionVisitor binaryVisitor_;
  UnaryExpressionVisitor unaryVisitor_;
  PrimaryExpressionVisitor primaryVisitor_;
  CallExpressionVisitor callVisitor_;
  CastExpressionVisitor castVisitor_;
};

} // namespace visitors
