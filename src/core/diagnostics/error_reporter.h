/*****************************************************************************
 * File: error_reporter.h
 * Description: Error and diagnostic reporting system for the compiler.
 *****************************************************************************/

#pragma once
#include "../common/common_types.h"
#include <vector>

namespace core {

struct Diagnostic {
  enum class Severity {
    Error,   // Fatal errors that prevent compilation
    Warning, // Potential issues that don't stop compilation
    Info     // Informational messages for the user
  };

  Diagnostic(Severity sev, const SourceLocation &loc, String msg, String c = "")
      : severity(sev), location(loc), message(std::move(msg)),
        code(std::move(c)) {}

  Severity severity;       // Message severity level
  SourceLocation location; // Source code location
  String message;          // Descriptive message
  String code;             // Optional diagnostic code (e.g., "E001")
};

class ErrorReporter {
public:
  // Report different types of diagnostics
  void error(const SourceLocation &location, const String &message,
             const String &code = "");

  void warning(const SourceLocation &location, const String &message,
               const String &code = "");

  void info(const SourceLocation &location, const String &message,
            const String &code = "");

  // Access collected diagnostics
  const std::vector<Diagnostic> &getDiagnostics() const { return diagnostics_; }

  // Error state queries
  bool hasErrors() const { return errorCount_ > 0; }
  int errorCount() const { return errorCount_; }

  // Diagnostic management
  void clear();
  void printAllErrors() const;

private:
  std::vector<Diagnostic> diagnostics_; // All collected diagnostics
  int errorCount_ = 0;                  // Number of errors encountered

  // Common reporting logic
  void report(Diagnostic::Severity severity, const SourceLocation &location,
              const String &message, const String &code);
};

} // namespace core