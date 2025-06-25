#include "file_utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace core::utils {

std::optional<String> FileUtils::readFile(const String &path) {
  // Open file in input mode
  std::ifstream file(path);

  // Check if file opening was successful
  if (!file) {
    return std::nullopt;
  }

  // Read entire file into a string stream
  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

bool FileUtils::writeFile(const String &path, const String &content) {
  // Open file in output mode
  std::ofstream file(path);

  // Check if file opening was successful
  if (!file) {
    return false;
  }

  // Write content to file
  file << content;

  // Verify write operation was successful
  return file.good();
}

bool FileUtils::exists(const String &path) {
  // Use std::filesystem to check file/directory existence
  return std::filesystem::exists(path);
}

String FileUtils::getExtension(const String &path) {
  // Find the last occurrence of '.'
  auto pos = path.find_last_of('.');

  // Return extension if found, otherwise empty string
  return pos == String::npos ? "" : path.substr(pos + 1);
}

bool FileUtils::createDirectories(const String &path) {
  // Create directories recursively
  return std::filesystem::create_directories(path);
}

} // namespace core::utils