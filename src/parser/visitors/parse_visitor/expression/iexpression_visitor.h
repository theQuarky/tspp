#pragma once
#include "parser/nodes/expression_nodes.h"
#include "parser/nodes/type_nodes.h"

namespace visitors {

class IExpressionVisitor {
public:
  virtual ~IExpressionVisitor() = default;

  // Core parsing methods required by sub-visitors
  virtual nodes::ExpressionPtr parseExpression() = 0;
  virtual nodes::ExpressionPtr parsePrimary() = 0;
  virtual nodes::ExpressionPtr parseUnary() = 0;
  virtual nodes::TypePtr parseType() = 0;
  virtual nodes::ExpressionPtr parseNewExpression() = 0;
  virtual nodes::ExpressionPtr parseFunctionExpression() = 0;
};

} // namespace visitors