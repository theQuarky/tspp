#include "codegen_options.h"
#include <cstdlib> // For getenv
#include <sstream> // For stringstream
#include <string>

namespace codegen {

CodeGenOptions::CodeGenOptions()
    : optimizationLevel_(OptimizationLevel::O2), targetArch_(TargetArch::AUTO),
      outputFormat_(OutputFormat::LLVM_IR), outputFilename_("output"),
      moduleName_("tspp_module"), debugInfo_(false), pic_(true), simd_(true),
      fastMath_(false), stackSize_(8 * 1024 * 1024) // 8 MB default stack size
{
  // Try to detect target architecture from environment
  detectTargetArch();

  // Set appropriate file extension based on output format
  updateFileExtension();
}

void CodeGenOptions::detectTargetArch() {
  if (targetArch_ != TargetArch::AUTO) {
    return; // User has explicitly set the arch
  }

  // Check for common environment variables that might indicate architecture
  const char *arch = std::getenv("PROCESSOR_ARCHITECTURE");
  if (arch) {
    std::string archStr(arch);
    if (archStr.find("AMD64") != std::string::npos ||
        archStr.find("x86_64") != std::string::npos) {
      targetArch_ = TargetArch::X86_64;
      return;
    }
    if (archStr.find("x86") != std::string::npos) {
      targetArch_ = TargetArch::X86;
      return;
    }
    if (archStr.find("ARM") != std::string::npos) {
      // Check for 64-bit ARM
      const char *variant = std::getenv("PROCESSOR_ARCHITEW6432");
      if (variant && std::string(variant).find("ARM64") != std::string::npos) {
        targetArch_ = TargetArch::AARCH64;
      } else {
        targetArch_ = TargetArch::ARM;
      }
      return;
    }
  }

  // Default to x86_64 if detection fails
  targetArch_ = TargetArch::X86_64;
}

void CodeGenOptions::updateFileExtension() {
  // First remove any existing extension
  size_t dotPos = outputFilename_.find_last_of('.');
  if (dotPos != std::string::npos) {
    outputFilename_ = outputFilename_.substr(0, dotPos);
  }

  // Add appropriate extension
  switch (outputFormat_) {
  case OutputFormat::LLVM_IR:
    outputFilename_ += ".ll";
    break;
  case OutputFormat::LLVM_BC:
    outputFilename_ += ".bc";
    break;
  case OutputFormat::ASSEMBLY:
    outputFilename_ += ".s";
    break;
  case OutputFormat::OBJECT:
    outputFilename_ += ".o";
    break;
  case OutputFormat::EXECUTABLE:
// No extension on Unix-like systems, .exe on Windows
#ifdef _WIN32
    outputFilename_ += ".exe";
#endif
    break;
  }
}

void CodeGenOptions::setOutputFormat(OutputFormat format) {
  outputFormat_ = format;
  updateFileExtension();
}

void CodeGenOptions::setOutputFilename(const std::string &filename) {
  outputFilename_ = filename;
  updateFileExtension();
}

std::string CodeGenOptions::optimizationLevelToString(OptimizationLevel level) {
  switch (level) {
  case OptimizationLevel::O0:
    return "O0";
  case OptimizationLevel::O1:
    return "O1";
  case OptimizationLevel::O2:
    return "O2";
  case OptimizationLevel::O3:
    return "O3";
  case OptimizationLevel::Os:
    return "Os";
  case OptimizationLevel::Oz:
    return "Oz";
  default:
    return "Unknown";
  }
}

std::string CodeGenOptions::targetArchToString(TargetArch arch) {
  switch (arch) {
  case TargetArch::X86:
    return "x86";
  case TargetArch::X86_64:
    return "x86-64";
  case TargetArch::ARM:
    return "arm";
  case TargetArch::AARCH64:
    return "aarch64";
  case TargetArch::WASM:
    return "wasm";
  case TargetArch::AUTO:
    return "auto";
  default:
    return "unknown";
  }
}

std::string CodeGenOptions::outputFormatToString(OutputFormat format) {
  switch (format) {
  case OutputFormat::LLVM_IR:
    return "LLVM IR";
  case OutputFormat::LLVM_BC:
    return "LLVM Bitcode";
  case OutputFormat::ASSEMBLY:
    return "Assembly";
  case OutputFormat::OBJECT:
    return "Object File";
  case OutputFormat::EXECUTABLE:
    return "Executable";
  default:
    return "Unknown";
  }
}

std::string CodeGenOptions::toString() const {
  std::stringstream ss;
  ss << "Code Generation Options:\n";
  ss << "  Optimization Level: "
     << optimizationLevelToString(optimizationLevel_) << "\n";
  ss << "  Target Architecture: " << targetArchToString(targetArch_) << "\n";
  ss << "  Output Format: " << outputFormatToString(outputFormat_) << "\n";
  ss << "  Output Filename: " << outputFilename_ << "\n";
  ss << "  Module Name: " << moduleName_ << "\n";
  ss << "  Debug Info: " << (debugInfo_ ? "Enabled" : "Disabled") << "\n";
  ss << "  PIC: " << (pic_ ? "Enabled" : "Disabled") << "\n";
  ss << "  SIMD: " << (simd_ ? "Enabled" : "Disabled") << "\n";
  ss << "  Fast Math: " << (fastMath_ ? "Enabled" : "Disabled") << "\n";
  ss << "  Stack Size: " << stackSize_ << " bytes\n";

  if (!targetOptions_.empty()) {
    ss << "  Target Options:\n";
    for (const auto &option : targetOptions_) {
      ss << "    " << option << "\n";
    }
  }

  return ss.str();
}

} // namespace codegen