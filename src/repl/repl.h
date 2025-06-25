#pragma once
#include "../core/diagnostics/error_reporter.h"
#include <string>

namespace repl {

class Repl {
public:
  explicit Repl(core::ErrorReporter &errorReporter);
  void start();

private:
  core::ErrorReporter &errorReporter_;
  bool showTokens_ = false; // Added flag for token output
  bool showAst_ = true;     // Added flag for AST output

  void processLine(const std::string &line);
  void printWelcome();
};

} // namespace repl