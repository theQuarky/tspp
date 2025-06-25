/*****************************************************************************
 * File: base_interface.h
 * Description: Base interface for parser visitors
 *****************************************************************************/

#pragma once

namespace interface {

/**
 * @class BaseInterface
 * @brief Base interface that all AST visitors must implement
 *
 * Defines the core contract for visitors in the parser system.
 * Each concrete visitor (parse visitor, type checker, etc.) must
 * implement these methods.
 */
class BaseInterface {
public:
  virtual ~BaseInterface() = default;

  /**
   * @brief Entry point for parsing phase
   * @return Success status of the parsing operation
   */
  virtual bool visitParse() = 0;

protected:
  BaseInterface() = default;
};

} // namespace interface