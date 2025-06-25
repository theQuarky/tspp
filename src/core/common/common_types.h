/*****************************************************************************
 * File: common_types.h
 * Description: Core type definitions and utilities used throughout the compiler
 *
 * Contains:
 * - Basic type aliases
 * - Source location tracking
 * - Value storage and manipulation
 * - Error handling and result types
 *****************************************************************************/

#pragma once
#include <fstream>
#include <sstream>
#include <string>

namespace core {

/*****************************************************************************
 * Basic Type Aliases
 *****************************************************************************/
using Int = int;
using Float = float;
using Bool = bool;
using String = std::string;

/*****************************************************************************
 * Source Location & Error Display
 *****************************************************************************/
class SourceLocation {
public:
  // Constants for error highlighting
  static constexpr const char *RED = "\033[1;31m";
  static constexpr const char *RESET = "\033[0m";

  // Constructors matching TokenLocation interface
  SourceLocation(unsigned int ln = 0, unsigned int col = 0,
                 std::string file = "")
      : line_(ln), column_(col), filename_(std::move(file)) {
    loadLineContent();
  }

  // TokenLocation compatible interface
  unsigned int getLine() const { return line_; }
  unsigned int getColumn() const { return column_; }
  const std::string &getFilename() const { return filename_; }

  // Extended functionality
  const std::string &getLineContent() const { return line_content_; }

  // Comparison operator
  bool operator==(const SourceLocation &other) const {
    return filename_ == other.filename_ && line_ == other.line_ &&
           column_ == other.column_;
  }

  // Format location for error display
  std::string toString() const {
    std::stringstream output;
    // File location
    output << filename_ << ":" << line_ << ":" << column_ << "\n";

    // Show line content with error pointer if available
    if (!line_content_.empty()) {
      output << line_content_ << "\n";
      std::string pointer_line(column_ - 1, ' ');
      output << pointer_line << RED << "^" << RESET;
    }

    return output.str();
  }

  // Reload line content (useful if file changes)
  void reloadContent() { loadLineContent(); }

private:
  unsigned int line_;        // Line number (1-based)
  unsigned int column_;      // Column number (1-based)
  std::string filename_;     // Source file name
  std::string line_content_; // Content of the error line

  // Load the source line for error display
  void loadLineContent() {
    std::ifstream file(filename_);
    if (!file.is_open()) {
      line_content_ = "";
      return;
    }

    std::string current_line;
    unsigned int current_line_num = 1;

    while (std::getline(file, current_line)) {
      if (current_line_num == line_) {
        line_content_ = current_line;
        break;
      }
      current_line_num++;
    }

    file.close();
  }
};

/*****************************************************************************
 * Value Storage
 *****************************************************************************/
union LiteralValue {
  Int int_value;
  Float float_value;
  Bool bool_value;

  // Value constructors
  LiteralValue() : int_value(0) {}
  explicit LiteralValue(Int v) : int_value(v) {}
  explicit LiteralValue(Float v) : float_value(v) {}
  explicit LiteralValue(Bool v) : bool_value(v) {}
};

/*****************************************************************************
 * Error Handling
 *****************************************************************************/
struct Error {
  String message;
  SourceLocation location;

  Error(String msg, SourceLocation loc)
      : message(std::move(msg)), location(std::move(loc)) {}

  String toString() const { return location.toString() + ": " + message; }
};

/*****************************************************************************
 * Result Type
 *****************************************************************************/
template <typename T> class Result {
private:
  T value_;
  Error error_;
  Bool has_error_;

public:
  // Constructors for success and failure cases
  Result(T value) : value_(std::move(value)), has_error_(false) {}
  Result(Error error) : error_(std::move(error)), has_error_(true) {}

  // Status checks
  Bool hasError() const { return has_error_; }
  const T &getValue() const { return value_; }
  const Error &getError() const { return error_; }

  // Implicit bool conversion for quick error checks
  operator bool() const { return !has_error_; }
};

} // namespace core