#pragma once
#include "./type_check_visitor/type_check_visitor.h" // Added include for type checker
#include "core/diagnostics/error_reporter.h"
#include "parser/ast.h"
#include "parser/visitors/parse_visitor/base/base_parse_visitor.h"
#include "tokens/stream/token_stream.h"
#include <memory>

namespace visitors {

/**
 * Main visitor interface that coordinates different visitor types.
 * Acts as a facade for different visitor implementations.
 */
class BaseVisitor {
public:
  explicit BaseVisitor(tokens::TokenStream &tokens,
                       core::ErrorReporter &errorReporter)
      : tokens_(tokens), errorReporter_(errorReporter),
        parseVisitor_(
            std::make_unique<BaseParseVisitor>(tokens_, errorReporter_)),
        // Initialize the type check visitor
        typeCheckVisitor_(
            std::make_unique<TypeCheckVisitor>(
                errorReporter_)) {}

  // Prevent copying but allow moving
  BaseVisitor(const BaseVisitor &) = delete;
  BaseVisitor &operator=(const BaseVisitor &) = delete;
  BaseVisitor(BaseVisitor &&) = default;
  BaseVisitor &operator=(BaseVisitor &&) = default;
  ~BaseVisitor() = default;

  // Entry point for parsing phase
  bool parse() {
    bool parseSuccess = parseVisitor_->visitParse();

    // Copy AST nodes from parse visitor if successful
    if (parseSuccess) {
      const auto &nodes = parseVisitor_->getNodes();
      for (const auto &node : nodes) {
        ast_.addNode(node);
      }
    }

    return parseSuccess;
  }

  // New method: Entry point for type checking phase
  bool typeCheck() {
    // Ensure we have an AST to type check
    if (ast_.getNodes().empty()) {
      errorReporter_.error(tokens_.peek().getLocation(),
                           "Cannot perform type checking without a parsed AST");
      return false;
    }

    // Perform type checking on the AST
    return typeCheckVisitor_->checkAST(ast_);
  }

  // New method: Run all compiler phases in sequence
  bool compile() {
    // Phase 1: Parsing
    if (!parse()) {
      return false;
    }

    // Phase 2: Type checking
    if (!typeCheck()) {
      return false;
    }

    // Add more phases here as they're implemented
    // Phase 3: Semantic analysis
    // Phase 4: Code generation

    return true;
  }

  // AST management
  void addNode(nodes::NodePtr node) { ast_.addNode(std::move(node)); }
  const parser::AST &getAST() { return ast_; }

private:
  tokens::TokenStream &tokens_;
  core::ErrorReporter &errorReporter_;
  parser::AST ast_;
  std::unique_ptr<BaseParseVisitor> parseVisitor_;
  // Added member for type checking
  std::unique_ptr<TypeCheckVisitor>
      typeCheckVisitor_;
};

} // namespace visitors