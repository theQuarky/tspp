/*****************************************************************************
 * File: parser.cpp
 * Description: Implementation of TSPP parser
 *****************************************************************************/

#include "parser.h"

namespace parser {

Parser::Parser(std::vector<tokens::Token> tokens,
               core::ErrorReporter &errorReporter)
    : tokens_(std::move(tokens)), errorReporter_(errorReporter), ast_(),
      visitor_(
          std::make_unique<visitors::BaseVisitor>(tokens_, errorReporter_)) {}

bool Parser::parse() {
  try {
    // Clear previous parse results
    ast_.clear();
    errorReporter_.clear();

    // Start the parsing and type checking process
    if (!visitor_->compile()) {  // Changed from parse() to compile()
      return false;
    }

    // Get the AST from the visitor
    ast_ = visitor_->getAST();

    // Check for any errors that occurred during parsing or type checking
    return !hasErrors();
  } catch (const std::exception &e) {
    errorReporter_.error(tokens_.peek().getLocation(),
                         std::string("Unexpected error during compilation: ") +
                             e.what());
    return false;
  } catch (...) {
    errorReporter_.error(tokens_.peek().getLocation(),
                         "Unknown error occurred during compilation");
    return false;
  }
}

} // namespace parser