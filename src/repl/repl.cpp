#include "repl.h"
#include "../core/utils/log_utils.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include <iostream>

namespace repl {

Repl::Repl(core::ErrorReporter &errorReporter)
    : errorReporter_(errorReporter) {}

void Repl::printWelcome() {
  std::cout << "TSPP REPL v0.1.0\n";
  std::cout << "Type '.exit' to exit\n";
  std::cout << "Type '.tokens' to toggle token output\n";
  std::cout << "Type '.ast' to toggle AST output\n\n";
}

void Repl::processLine(const std::string &line) {
  try {
    // Reset error reporter for new input
    errorReporter_.clear();

    // Lexical analysis
    lexer::Lexer lexer(line, "<repl>");
    auto tokens = lexer.tokenize();

    if (tokens.empty()) {
      return;
    }

    // Print tokens if enabled
    if (showTokens_) {
      printTokenStream(tokens);
      std::cout << "\n";
    }

    // Parsing
    parser::Parser parser(std::move(tokens), errorReporter_);
    if (!parser.parse()) {
      if (errorReporter_.hasErrors()) {
        errorReporter_.printAllErrors();
      }
      return;
    }

    // Print AST if enabled
    if (showAst_) {
      const auto &ast = parser.getAST();
      // TODO: Add AST printer visitor
      // parser::ASTPrinterVisitor printer;
      // printer.print(ast);
      // std::cout << printer.getOutput() << "\n";
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void Repl::start() {
  printWelcome();
  std::string line;

  while (true) {
    std::cout << ">> ";
    if (!std::getline(std::cin, line) || line == ".exit") {
      break;
    }

    // Handle REPL commands
    if (line == ".tokens") {
      showTokens_ = !showTokens_;
      std::cout << "Token output " << (showTokens_ ? "enabled" : "disabled")
                << "\n";
      continue;
    }
    if (line == ".ast") {
      showAst_ = !showAst_;
      std::cout << "AST output " << (showAst_ ? "enabled" : "disabled") << "\n";
      continue;
    }

    processLine(line);
  }
}

} // namespace repl