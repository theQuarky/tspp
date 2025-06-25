/*****************************************************************************
 * File: error_reporter.cpp
 * Description: Implementation of error reporting and diagnostic handling.
 *****************************************************************************/

#include "error_reporter.h"
#include <iostream>

namespace core {

namespace {
const String RED = "\033[1;31m";    // Error
const String YELLOW = "\033[1;33m"; // Warning
const String BLUE = "\033[1;34m";   // Info
const String RESET = "\033[0m";     // Reset color
} // namespace

void ErrorReporter::error(const SourceLocation &location, const String &message,
                          const String &code) {
  report(Diagnostic::Severity::Error, location, message, code);
  errorCount_++;
}

void ErrorReporter::warning(const SourceLocation &location,
                            const String &message, const String &code) {
  report(Diagnostic::Severity::Warning, location, message, code);
}

void ErrorReporter::info(const SourceLocation &location, const String &message,
                         const String &code) {
  report(Diagnostic::Severity::Info, location, message, code);
}

void ErrorReporter::clear() {
  diagnostics_.clear();
  errorCount_ = 0;
}

void ErrorReporter::report(Diagnostic::Severity severity,
                           const SourceLocation &location,
                           const String &message, const String &code) {
  // Store diagnostic
  diagnostics_.emplace_back(severity, location, message, code);
  auto &diag = diagnostics_.back();

  // Select color based on severity
  const String &color = severity == Diagnostic::Severity::Error     ? RED
                        : severity == Diagnostic::Severity::Warning ? YELLOW
                                                                    : BLUE;

  const char *sevStr = severity == Diagnostic::Severity::Error     ? "error"
                       : severity == Diagnostic::Severity::Warning ? "warning"
                                                                   : "info";

  // Use toString() for location display
  std::cerr << location.toString();

  // Add severity and code
  std::cerr << color << sevStr << RESET;
  if (!code.empty()) {
    std::cerr << "[" << code << "]";
  }

  // Add message
  std::cerr << ": " << message << "\n";
}

void ErrorReporter::printAllErrors() const {
  for (const auto &diag : diagnostics_) {
    const String &color = diag.severity == Diagnostic::Severity::Error ? RED
                          : diag.severity == Diagnostic::Severity::Warning
                              ? YELLOW
                              : BLUE;

    const char *sevStr = diag.severity == Diagnostic::Severity::Error ? "error"
                         : diag.severity == Diagnostic::Severity::Warning
                             ? "warning"
                             : "info";

    // Use toString() for formatting
    std::cerr << diag.location.toString();

    // Add severity and code
    std::cerr << color << sevStr << RESET;
    if (!diag.code.empty()) {
      std::cerr << " [" << diag.code << "]";
    }

    // Add message
    std::cerr << ": " << diag.message << "\n";
  }
}

} // namespace core