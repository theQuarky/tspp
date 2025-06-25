#include "codegen/codegen_options.h"
#include "codegen/llvm/llvm_code_gen.h"
#include "core/diagnostics/error_reporter.h"
#include "core/utils/file_utils.h"
#include "core/utils/log_utils.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "repl/repl.h"
#include <iostream>

int main(int argc, char *argv[]) {
  try {
    core::ErrorReporter errorReporter;

    if (argc == 1) {
      repl::Repl repl(errorReporter);
      repl.start();
      return 0;
    }

    const std::string filePath = argv[1];

    if (core::utils::FileUtils::getExtension(filePath) != "tspp") {
      std::cerr << "Error: File must have .tspp extension\n";
      return 1;
    }

    if (!core::utils::FileUtils::exists(filePath)) {
      std::cerr << "Error: File does not exist: " << filePath << "\n";
      return 1;
    }

    auto sourceCode = core::utils::FileUtils::readFile(filePath);
    if (!sourceCode) {
      std::cerr << "Error: Could not read file: " << filePath << "\n";
      return 1;
    }

    // Lexical analysis
    lexer::Lexer lexer(*sourceCode, filePath);
    auto tokens = lexer.tokenize();

    if (tokens.empty()) {
      std::cerr << "Fatal errors occurred during lexical analysis.\n";
      return 1;
    }

    // Print tokens if you want to see the lexer output
    // printTokenStream(tokens);

    // Parsing
    parser::Parser parser(std::move(tokens), errorReporter);
    if (!parser.parse()) {
      return 1;
    }

    // Get AST for next phase
    const auto &ast = parser.getAST();
    // TODO: Next phases (type checking, optimization, code generation)
    if (!ast.getNodes().empty()) {
      // Create code generator
      codegen::CodeGenOptions options;
      options.setOutputFilename(filePath + ".ll"); // Output LLVM IR

      codegen::LLVMCodeGen codeGen(errorReporter);

      // if (codeGen.generateCode(ast)) {
      //   // Need to pass the options filename to the code generator
      //   if (codeGen.writeToFile(options.getOutputFilename())) {
      //     std::cout << "Code generation successful. Output written to "
      //               << options.getOutputFilename() << std::endl;
      //   } else {
      //     std::cerr << "Failed to write output file." << std::endl;
      //     return 1;
      //   }

      //   std::cout << "Executing program..." << std::endl;
      //   if (!codeGen.executeCode()) {
      //     std::cerr << "Execution failed." << std::endl;
      //     return 1;
      //   }
      if (codeGen.generateCode(ast)) {
        if (codeGen.writeToFile(options.getOutputFilename())) {
          std::cout << "Code generation successful. Output written to "
                    << options.getOutputFilename() << std::endl;

          // Disable execution for now to avoid crashes
          // You can view the generated LLVM IR in the .ll file
        } else {
          std::cerr << "Failed to write output file." << std::endl;
          return 1;
        }

      } else {
        std::cerr << "Code generation failed." << std::endl;
        return 1;
      }
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << "\n";
    return 1;
  }
}