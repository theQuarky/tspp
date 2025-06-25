/*****************************************************************************
 * File: ast.h
 * Description: Abstract Syntax Tree container
 *****************************************************************************/

#pragma once
#include "nodes/base_node.h"
#include <vector>

namespace parser {

class AST {
public:
  // Add a node to the AST (usually called during parsing)
  void addNode(nodes::NodePtr node) { nodes_.push_back(std::move(node)); }

  // Access nodes
  const std::vector<nodes::NodePtr> &getNodes() const { return nodes_; }

  // Clear the AST
  void clear() { nodes_.clear(); }

private:
  std::vector<nodes::NodePtr> nodes_; // Top-level nodes in the AST
};

} // namespace parser