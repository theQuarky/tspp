#include "string_utils.h"
#include <sstream>

namespace core::utils {

std::vector<String> StringUtils::split(const String &str, char delimiter) {
  std::vector<String> tokens;
  std::istringstream stream(str);
  String token;

  // Extract tokens using getline with specified delimiter
  while (std::getline(stream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

String StringUtils::join(const std::vector<String> &parts,
                         const String &delimiter) {
  // Handle empty input
  if (parts.empty()) {
    return "";
  }

  // Build result string efficiently using ostringstream
  std::ostringstream result;
  result << parts[0];

  // Append remaining parts with delimiter
  for (size_t i = 1; i < parts.size(); ++i) {
    result << delimiter << parts[i];
  }
  return result.str();
}

String StringUtils::trim(const String &str) {
  // Find first non-whitespace character
  auto first = str.find_first_not_of(" \t\n\r");
  if (first == String::npos) {
    return "";
  }

  // Find last non-whitespace character
  auto last = str.find_last_not_of(" \t\n\r");

  // Return substring between first and last non-whitespace
  return str.substr(first, last - first + 1);
}

String StringUtils::replace(String str, const String &from, const String &to) {
  size_t pos = 0;

  // Replace all occurrences of the substring
  while ((pos = str.find(from, pos)) != String::npos) {
    str.replace(pos, from.length(), to);
    pos += to.length();
  }
  return str;
}

bool StringUtils::startsWith(const String &str, const String &prefix) {
  // Check if string is long enough and starts with prefix
  return str.length() >= prefix.length() &&
         str.compare(0, prefix.length(), prefix) == 0;
}

bool StringUtils::endsWith(const String &str, const String &suffix) {
  // Check if string is long enough and ends with suffix
  return str.length() >= suffix.length() &&
         str.compare(str.length() - suffix.length(), suffix.length(), suffix) ==
             0;
}

} // namespace core::utils