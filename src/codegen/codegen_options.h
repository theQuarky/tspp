#pragma once
#include <string>
#include <vector>

namespace codegen {

/**
 * @enum OptimizationLevel
 * @brief LLVM optimization levels
 */
enum class OptimizationLevel {
  O0, // No optimization
  O1, // Basic optimizations
  O2, // Moderate optimizations
  O3, // Aggressive optimizations
  Os, // Optimize for size
  Oz  // Optimize for size aggressively
};

/**
 * @enum TargetArch
 * @brief Target architecture options
 */
enum class TargetArch {
  X86,     // x86 32-bit
  X86_64,  // x86 64-bit
  ARM,     // ARM 32-bit
  AARCH64, // ARM 64-bit
  WASM,    // WebAssembly
  AUTO     // Auto-detect host architecture
};

/**
 * @enum OutputFormat
 * @brief Output file format options
 */
enum class OutputFormat {
  LLVM_IR,   // LLVM Intermediate Representation text
  LLVM_BC,   // LLVM Bitcode
  ASSEMBLY,  // Target assembly
  OBJECT,    // Object file
  EXECUTABLE // Executable file
};

/**
 * @class CodeGenOptions
 * @brief Configuration options for code generation
 *
 * This class encapsulates all configuration options that affect
 * how code is generated and optimized.
 */
class CodeGenOptions {
public:
  /**
   * @brief Constructs options with default values
   */
  CodeGenOptions();

  /**
   * @brief Sets the optimization level
   * @param level The optimization level
   */
  void setOptimizationLevel(OptimizationLevel level) {
    optimizationLevel_ = level;
  }

  /**
   * @brief Gets the optimization level
   * @return The optimization level
   */
  OptimizationLevel getOptimizationLevel() const { return optimizationLevel_; }

  /**
   * @brief Sets the target architecture
   * @param arch The target architecture
   */
  void setTargetArch(TargetArch arch) { targetArch_ = arch; }

  /**
   * @brief Gets the target architecture
   * @return The target architecture
   */
  TargetArch getTargetArch() const { return targetArch_; }

  /**
   * @brief Sets the output format
   * @param format The output format
   */
  void setOutputFormat(OutputFormat format);

  /**
   * @brief Gets the output format
   * @return The output format
   */
  OutputFormat getOutputFormat() const { return outputFormat_; }

  /**
   * @brief Sets the output filename
   * @param filename The output filename
   */
  void setOutputFilename(const std::string &filename);

  /**
   * @brief Gets the output filename
   * @return The output filename
   */
  const std::string &getOutputFilename() const { return outputFilename_; }

  /**
   * @brief Enables or disables debug information
   * @param enable Whether to enable debug information
   */
  void setDebugInfo(bool enable) { debugInfo_ = enable; }

  /**
   * @brief Checks if debug information is enabled
   * @return True if debug information is enabled
   */
  bool isDebugInfoEnabled() const { return debugInfo_; }

  /**
   * @brief Enables or disables position-independent code
   * @param enable Whether to enable position-independent code
   */
  void setPIC(bool enable) { pic_ = enable; }

  /**
   * @brief Checks if position-independent code is enabled
   * @return True if position-independent code is enabled
   */
  bool isPICEnabled() const { return pic_; }

  /**
   * @brief Enables or disables SIMD (vectorization) optimizations
   * @param enable Whether to enable SIMD optimizations
   */
  void setSIMD(bool enable) { simd_ = enable; }

  /**
   * @brief Checks if SIMD optimizations are enabled
   * @return True if SIMD optimizations are enabled
   */
  bool isSIMDEnabled() const { return simd_; }

  /**
   * @brief Sets the module name
   * @param name The module name
   */
  void setModuleName(const std::string &name) { moduleName_ = name; }

  /**
   * @brief Gets the module name
   * @return The module name
   */
  const std::string &getModuleName() const { return moduleName_; }

  /**
   * @brief Adds a target-specific option
   * @param option The option to add
   */
  void addTargetOption(const std::string &option) {
    targetOptions_.push_back(option);
  }

  /**
   * @brief Gets the target-specific options
   * @return The target-specific options
   */
  const std::vector<std::string> &getTargetOptions() const {
    return targetOptions_;
  }

  /**
   * @brief Sets whether to use LLVM fast math flags
   * @param enable Whether to enable fast math
   */
  void setFastMath(bool enable) { fastMath_ = enable; }

  /**
   * @brief Checks if fast math is enabled
   * @return True if fast math is enabled
   */
  bool isFastMathEnabled() const { return fastMath_; }

  /**
   * @brief Sets the stack size for stack variables
   * @param size The stack size in bytes
   */
  void setStackSize(size_t size) { stackSize_ = size; }

  /**
   * @brief Gets the stack size for stack variables
   * @return The stack size in bytes
   */
  size_t getStackSize() const { return stackSize_; }

  /**
   * @brief Gets a string representation of the options
   * @return String representation
   */
  std::string toString() const;

private:
  /**
   * @brief Detects the target architecture from environment
   */
  void detectTargetArch();

  /**
   * @brief Updates the file extension based on output format
   */
  void updateFileExtension();

  /**
   * @brief Converts optimization level to string
   * @param level The optimization level
   * @return String representation
   */
  static std::string optimizationLevelToString(OptimizationLevel level);

  /**
   * @brief Converts target architecture to string
   * @param arch The target architecture
   * @return String representation
   */
  static std::string targetArchToString(TargetArch arch);

  /**
   * @brief Converts output format to string
   * @param format The output format
   * @return String representation
   */
  static std::string outputFormatToString(OutputFormat format);

  OptimizationLevel optimizationLevel_;    // Optimization level
  TargetArch targetArch_;                  // Target architecture
  OutputFormat outputFormat_;              // Output file format
  std::string outputFilename_;             // Output file path
  std::string moduleName_;                 // LLVM module name
  std::vector<std::string> targetOptions_; // Target-specific options
  bool debugInfo_;                         // Whether to include debug info
  bool pic_;                               // Position-independent code
  bool simd_;                              // Enable SIMD optimizations
  bool fastMath_;                          // Enable fast math flags
  size_t stackSize_;                       // Stack size for stack variables
};

} // namespace codegen