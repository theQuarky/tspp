#pragma once

#include "../common/common_types.h"
#include <vector>

namespace core::utils {

/**
 * @class StringUtils
 * @brief Provides static utility functions for string manipulation
 *
 * This class offers a collection of methods to perform common string operations
 * such as splitting, joining, trimming, replacing, and checking string
 * properties.
 */
class StringUtils {
public:
  // String Parsing and Transformation
  /**
   * @brief Splits a string into tokens using a specified delimiter
   * @param str String to split
   * @param delimiter Character used to separate tokens
   * @return Vector of string tokens
   * @throws None
   */
  static std::vector<String> split(const String &str, char delimiter);

  /**
   * @brief Joins a vector of strings with a specified delimiter
   * @param parts Vector of strings to join
   * @param delimiter String used to connect parts
   * @return Joined string, or empty string if input is empty
   * @throws None
   */
  static String join(const std::vector<String> &parts, const String &delimiter);

  /**
   * @brief Removes whitespace from both ends of a string
   * @param str String to trim
   * @return Trimmed string, or empty string if input is only whitespace
   * @throws None
   */
  static String trim(const String &str);

  /**
   * @brief Replaces all occurrences of a substring with another
   * @param str Original string to modify
   * @param from Substring to replace
   * @param to Replacement substring
   * @return Modified string with all replacements
   * @throws None
   */
  static String replace(String str, const String &from, const String &to);

  // String Property Checks
  /**
   * @brief Checks if a string starts with a given prefix
   * @param str String to check
   * @param prefix Prefix to match at the start
   * @return True if string starts with prefix, false otherwise
   * @throws None
   */
  static bool startsWith(const String &str, const String &prefix);

  /**
   * @brief Checks if a string ends with a given suffix
   * @param str String to check
   * @param suffix Suffix to match at the end
   * @return True if string ends with suffix, false otherwise
   * @throws None
   */
  static bool endsWith(const String &str, const String &suffix);
};

} // namespace core::utils