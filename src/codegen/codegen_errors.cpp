#include "codegen_errors.h"
#include <sstream>

namespace codegen {

// Constructor for CodeGenError
CodeGenError::CodeGenError(CodeGenErrorCode code, const std::string &message,
                           const core::SourceLocation &location)
    : code_(code), message_(message), location_(location) {}

// Generate string representation of an error
std::string CodeGenError::toString() const {
  std::stringstream ss;

  // Error code as number
  ss << "CG" << static_cast<int>(code_) << ": ";

  // Error message
  ss << message_;

  return ss.str();
}

// Constructor for CodeGenErrorReporter
CodeGenErrorReporter::CodeGenErrorReporter(core::ErrorReporter &coreReporter)
    : coreReporter_(coreReporter), errorCount_(0) {}

// Report a code generation error
void CodeGenErrorReporter::reportError(const CodeGenError &error) {
  errorCount_++;

  // Format the message with the code included
  std::string formattedMessage = error.toString();

  // Delegate to the core error reporter
  coreReporter_.error(error.getLocation(), formattedMessage,
                      "CG" + std::to_string(static_cast<int>(error.getCode())));
}

// Clear error state
void CodeGenErrorReporter::clear() {
  errorCount_ = 0;
  // Note: We don't clear the core reporter here as it may contain errors
  // from other phases of compilation
}

} // namespace codegen