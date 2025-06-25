#pragma once
#include "parser/nodes/declaration_nodes.h"
#include "parser/nodes/statement_nodes.h"

namespace visitors {

class IStatementVisitor {
public:
  virtual ~IStatementVisitor() = default;

  // Required parsing methods for all statement visitors
  virtual nodes::StmtPtr parseStatement() = 0;
  virtual nodes::BlockPtr parseBlock() = 0;
};

} // namespace visitors