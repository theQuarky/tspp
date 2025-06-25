/*****************************************************************************
 * File: parser.h
 * Description: Main parser interface for TSPP language
 *****************************************************************************/

#pragma once
#include "core/diagnostics/error_reporter.h"
#include "parser/ast.h"
#include "parser/visitors/base_visitor.h"
#include "tokens/stream/token_stream.h"

namespace parser {

class Parser {
public:
  explicit Parser(std::vector<tokens::Token> tokens,
                  core::ErrorReporter &errorReporter);

  // Parse the token stream and build AST
  bool parse();

  // Access the AST and errors
  const AST &getAST() const { return ast_; }
  bool hasErrors() const { return errorReporter_.hasErrors(); }
  const std::vector<core::Diagnostic> &getErrors() const {
    return errorReporter_.getDiagnostics();
  }

private:
  tokens::TokenStream tokens_;                // Token stream being parsed
  core::ErrorReporter &errorReporter_;        // Error reporting 
  AST ast_;                                   // AST being built
  std::unique_ptr<visitors::BaseVisitor> visitor_; // Main visitor for parsing
};

} // namespace parser