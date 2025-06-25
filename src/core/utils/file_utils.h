#pragma once

#include "../common/common_types.h"
#include <optional>

namespace core::utils {

/**
 * @class FileUtils
 * @brief Provides static utility functions for file system operations
 *
 * This class offers a collection of utility methods for file-related tasks
 * such as reading, writing, checking existence, and managing directories.
 */
class FileUtils {
public:
  // File Reading and Writing
  /**
   * @brief Reads the entire content of a file into a string
   * @param path Path to the file to read
   * @return Optional containing file content, or nullopt if reading fails
   * @throws None
   */
  static std::optional<String> readFile(const String &path);

  /**
   * @brief Writes string content to a file
   * @param path Path to the file to write
   * @param content Content to write to the file
   * @return True if writing was successful, false otherwise
   * @throws None
   */
  static bool writeFile(const String &path, const String &content);

  // File System Queries
  /**
   * @brief Checks if a file or directory exists at the given path
   * @param path Path to check for existence
   * @return True if the path exists, false otherwise
   * @throws None
   */
  static bool exists(const String &path);

  /**
   * @brief Extracts the file extension from a given path
   * @param path File path to extract extension from
   * @return File extension (without the dot), or empty string if no extension
   * @throws None
   */
  static String getExtension(const String &path);

  // Directory Management
  /**
   * @brief Creates directories recursively at the specified path
   * @param path Path of directories to create
   * @return True if directories were created or already exist, false on error
   * @throws None
   */
  static bool createDirectories(const String &path);
};

} // namespace core::utils