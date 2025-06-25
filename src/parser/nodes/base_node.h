/*****************************************************************************
 * File: base_node.h
 * Description: Base AST node definition used across all node types
 *****************************************************************************/

#pragma once
#include "core/common/common_types.h"
#include "parser/interfaces/base_interface.h"
#include <memory>

namespace nodes {

/**
 * @brief Base class for all AST nodes
 *
 * Provides common functionality for all nodes in the AST:
 * - Source location tracking for error reporting
 * - Visitor pattern support
 * - Memory management through smart pointers
 */
class BaseNode {
public:
  explicit BaseNode(const core::SourceLocation &loc) : location_(loc) {}

  virtual ~BaseNode() = default;

  // Get source location for error reporting
  const core::SourceLocation &getLocation() const { return location_; }

  // Visitor pattern support
  virtual bool accept(interface::BaseInterface *visitor) = 0;

protected:
  core::SourceLocation location_;
};

// Smart pointer type for nodes
using NodePtr = std::shared_ptr<BaseNode>;

} // namespace nodes