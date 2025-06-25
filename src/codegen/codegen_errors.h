#pragma once
#include "core/common/common_types.h"
#include "core/diagnostics/error_reporter.h"
#include <string>

namespace codegen {

/**
 * @enum CodeGenErrorCode
 * @brief Error codes specific to code generation
 *
 * This enum defines specific error codes for the code generation phase
 * to allow for structured error reporting and handling.
 */
enum class CodeGenErrorCode {
  // General errors
  UNKNOWN_ERROR = 1000,
  LLVM_INITIALIZATION_FAILED,
  MODULE_CREATION_FAILED,

  // Type-related errors
  TYPE_NOT_FOUND = 2000,
  INVALID_TYPE_CONVERSION,
  STRUCT_FIELD_NOT_FOUND,

  // Expression-related errors
  INVALID_BINARY_OPERATION = 3000,
  INVALID_UNARY_OPERATION,
  INVALID_CAST,
  DIVISION_BY_ZERO,

  // Function-related errors
  FUNCTION_NOT_FOUND = 4000,
  INVALID_RETURN_TYPE,
  PARAMETER_COUNT_MISMATCH,
  PARAMETER_TYPE_MISMATCH,

  // Variable-related errors
  VARIABLE_NOT_FOUND = 5000,
  REDEFINED_VARIABLE,
  UNINITIALIZED_VARIABLE,

  // Memory-related errors
  ALLOCATION_FAILED = 6000,
  NULL_POINTER_DEREFERENCE,
  MEMORY_LEAK,

  // I/O errors
  FILE_WRITE_FAILED = 7000,
  FILE_READ_FAILED,

  // Optimization errors
  OPTIMIZATION_FAILED = 8000
};

/**
 * @class CodeGenError
 * @brief Represents an error during code generation
 *
 * This class encapsulates error information from the code generation phase,
 * including a structured error code and message.
 */
class CodeGenError {
public:
  /**
   * @brief Constructs a code generation error
   * @param code Error code
   * @param message Descriptive error message
   * @param location Source location where the error occurred
   */
  CodeGenError(CodeGenErrorCode code, const std::string &message,
               const core::SourceLocation &location);

  /**
   * @brief Gets the error code
   * @return The error code
   */
  CodeGenErrorCode getCode() const { return code_; }

  /**
   * @brief Gets the error message
   * @return The error message
   */
  const std::string &getMessage() const { return message_; }

  /**
   * @brief Gets the error location
   * @return The error location
   */
  const core::SourceLocation &getLocation() const { return location_; }

  /**
   * @brief Gets a string representation of this error
   * @return String representation
   */
  std::string toString() const;

private:
  CodeGenErrorCode code_;         // Structured error code
  std::string message_;           // Descriptive error message
  core::SourceLocation location_; // Source location of the error
};

/**
 * @class CodeGenErrorReporter
 * @brief Reports and manages code generation errors
 *
 * This class extends the core error reporter to handle code generation-specific
 * errors and provide extra functionality for code generation diagnostics.
 */
class CodeGenErrorReporter {
public:
  /**
   * @brief Constructs an error reporter
   * @param coreReporter Core error reporter to delegate to
   */
  explicit CodeGenErrorReporter(core::ErrorReporter &coreReporter);

  /**
   * @brief Reports a code generation error
   * @param error The error to report
   */
  void reportError(const CodeGenError &error);

  /**
   * @brief Checks if any errors have been reported
   * @return True if errors have been reported
   */
  bool hasErrors() const { return errorCount_ > 0; }

  /**
   * @brief Gets the number of errors reported
   * @return Error count
   */
  int getErrorCount() const { return errorCount_; }

  /**
   * @brief Clears error state
   */
  void clear();

private:
  core::ErrorReporter &coreReporter_; // Core error reporter
  int errorCount_;                    // Count of code generation errors
};

} // namespace codegen