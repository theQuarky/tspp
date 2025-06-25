#pragma once
#include "core/diagnostics/error_reporter.h"
#include "llvm_context.h"
#include "llvm_function.h"
#include "llvm_optimizer.h"
#include "llvm_type_builder.h"
#include "llvm_value.h"
#include "parser/ast.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/nodes/expression_nodes.h"
#include "parser/nodes/statement_nodes.h"
#include "parser/nodes/type_nodes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace codegen {

/**
 * @class LLVMCodeGen
 * @brief Main LLVM code generator for TS++
 *
 * This class is responsible for traversing the AST and generating LLVM IR code.
 * It provides comprehensive error handling, proper memory management, and
 * supports the core language features including functions, variables,
 * expressions, and statements.
 */
class LLVMCodeGen {
public:
  /**
   * @brief Constructs a code generator
   * @param errorReporter Error reporter for diagnostics
   * @param moduleName Name for the LLVM module
   */
  LLVMCodeGen(core::ErrorReporter &errorReporter,
              const std::string &moduleName = "tspp_module");

  /**
   * @brief Generates code for an AST
   * @param ast The TS++ AST
   * @return True if code generation was successful
   */
  bool generateCode(const parser::AST &ast);

  /**
   * @brief Optimizes the generated code
   * @param level Optimization level
   */
  void optimize(OptimizationLevel level = OptimizationLevel::O2);

  /**
   * @brief Writes the generated code to a file
   * @param filename Path to the output file
   * @return True if the operation was successful
   */
  bool writeToFile(const std::string &filename) const;

  /**
   * @brief Gets the LLVM context
   * @return Reference to the LLVM context
   */
  LLVMContext &getContext() { return context_; }

  /**
   * @brief Executes the generated code using LLVM JIT
   * @return True if execution was successful
   */
  bool executeCode();

private:
  // External function declarations
  /**
   * @brief Declares external functions needed by generated code
   *
   * This includes standard library functions like printf, malloc, etc.
   */
  void declareExternalFunctions();

  // Top-level AST node visitors
  /**
   * @brief Processes a top-level declaration node
   * @param node The AST node to process
   * @return True if processing was successful
   */
  bool visitTopLevelDeclaration(const nodes::NodePtr &node);

  // Type system
  /**
   * @brief Pre-declares all types before code generation
   * @param ast The AST to scan for type declarations
   */
  void declareTypes(const parser::AST &ast);

  // Declaration visitors
  void visitGlobalDecl(const nodes::NodePtr &node);

  /**
   * @brief Processes a variable declaration
   * @param node The variable declaration node
   * @param isGlobal Whether this is a global variable
   * @return The generated LLVM value
   */
  LLVMValue visitVarDecl(const nodes::VarDeclNode *node, bool isGlobal = false);

  /**
   * @brief Processes a function declaration
   * @param node The function declaration node
   * @return True if successful
   */
  bool visitFunctionDecl(const nodes::FunctionDeclNode *node);

  /**
   * @brief Processes a global variable declaration
   * @param node The variable declaration node
   * @return True if successful
   */
  bool visitGlobalVarDecl(const nodes::VarDeclNode *node);

  void visitClassDecl(const nodes::ClassDeclNode *node);
  void visitNamespaceDecl(const nodes::NamespaceDeclNode *node);
  void visitEnumDecl(const nodes::EnumDeclNode *node);
  void visitInterfaceDecl(const nodes::InterfaceDeclNode *node);
  LLVMValue visitParameter(const nodes::ParameterNode *node);

  // Statement visitors
  /**
   * @brief Processes any statement node
   * @param node The statement node
   * @return The generated LLVM value
   */
  LLVMValue visitStmt(const nodes::StatementNode *node);

  LLVMValue visitExprStmt(const nodes::ExpressionStmtNode *node);

  /**
   * @brief Processes an expression statement at the top level
   * @param node The expression statement node
   * @return True if successful
   */
  bool visitExpressionStatement(const nodes::ExpressionStmtNode *node);

  /**
   * @brief Processes a block statement with proper scoping
   * @param node The block node
   * @return The generated LLVM value
   */
  LLVMValue visitBlock(const nodes::BlockNode *node);

  LLVMValue visitIfStmt(const nodes::IfStmtNode *node);
  LLVMValue visitWhileStmt(const nodes::WhileStmtNode *node);
  LLVMValue visitDoWhileStmt(const nodes::DoWhileStmtNode *node);
  LLVMValue visitForStmt(const nodes::ForStmtNode *node);
  LLVMValue visitForOfStmt(const nodes::ForOfStmtNode *node);
  LLVMValue visitReturnStmt(const nodes::ReturnStmtNode *node);
  LLVMValue visitBreakStmt(const nodes::BreakStmtNode *node);
  LLVMValue visitContinueStmt(const nodes::ContinueStmtNode *node);
  LLVMValue visitSwitchStmt(const nodes::SwitchStmtNode *node);
  LLVMValue visitTryStmt(const nodes::TryStmtNode *node);
  LLVMValue visitThrowStmt(const nodes::ThrowStmtNode *node);
  LLVMValue visitDeclStmt(const nodes::DeclarationStmtNode *node);

  /**
   * @brief Processes an assembly statement
   * @param node The assembly statement node
   * @return True if successful
   */
  bool visitAssemblyStatement(const nodes::AssemblyStmtNode *node);

  /**
   * @brief Processes a top-level statement
   * @param node The statement node
   * @return True if successful
   */
  bool visitTopLevelStatement(const nodes::StatementNode *node);

  // Expression visitors
  /**
   * @brief Processes any expression node
   * @param node The expression node
   * @return The generated LLVM value
   */
  LLVMValue visitExpr(const nodes::ExpressionNode *node);

  /**
   * @brief Processes a binary expression with proper type handling
   * @param node The binary expression node
   * @return The generated LLVM value
   */
  LLVMValue visitBinaryExpr(const nodes::BinaryExpressionNode *node);

  /**
   * @brief Processes a unary expression
   * @param node The unary expression node
   * @return The generated LLVM value
   */
  LLVMValue visitUnaryExpr(const nodes::UnaryExpressionNode *node);

  /**
   * @brief Processes a literal expression
   * @param node The literal expression node
   * @return The generated LLVM value
   */
  LLVMValue visitLiteralExpr(const nodes::LiteralExpressionNode *node);

  /**
   * @brief Processes an identifier expression
   * @param node The identifier expression node
   * @return The generated LLVM value
   */
  LLVMValue visitIdentifierExpr(const nodes::IdentifierExpressionNode *node);

  /**
   * @brief Processes a function call expression
   * @param node The call expression node
   * @return The generated LLVM value
   */
  LLVMValue visitCallExpr(const nodes::CallExpressionNode *node);

  LLVMValue visitMemberExpr(const nodes::MemberExpressionNode *node);
  LLVMValue visitIndexExpr(const nodes::IndexExpressionNode *node);

  /**
   * @brief Processes an assignment expression
   * @param node The assignment expression node
   * @return The generated LLVM value
   */
  LLVMValue visitAssignmentExpr(const nodes::AssignmentExpressionNode *node);

  LLVMValue visitNewExpr(const nodes::NewExpressionNode *node);
  LLVMValue visitCastExpr(const nodes::CastExpressionNode *node);
  LLVMValue visitArrayLiteral(const nodes::ArrayLiteralNode *node);

  // Main function creation
  /**
   * @brief Creates a default main function if none exists
   * @return Pointer to the created main function, or nullptr on failure
   */
  llvm::Function *createDefaultMainFunction();

  // Helper methods
  /**
   * @brief Reports an error with location information
   * @param location Source location of the error
   * @param message Error message
   */
  void error(const core::SourceLocation &location, const std::string &message);

  /**
   * @brief Reports a warning with location information
   * @param location Source location of the warning
   * @param message Warning message
   */
  void warning(const core::SourceLocation &location,
               const std::string &message);

  // String parsing helpers for assembly statements
  /**
   * @brief Parses escape sequences in string literals
   * @param input The input string with escape sequences
   * @return The parsed string with escape sequences resolved
   */
  std::string parseStringLiteral(const std::string &input);

  /**
   * @brief Checks if assembly code is a simple printf call
   * @param asmCode The assembly code to check
   * @return True if it's a simple printf call
   */
  bool isSimplePrintfCall(const std::string &asmCode);

  /**
   * @brief Extracts the string from a printf call
   * @param asmCode The assembly code containing the printf call
   * @return The extracted string
   */
  std::string extractPrintfString(const std::string &asmCode);

  // Loop management for break/continue
  /**
   * @brief Information about a loop for break/continue handling
   */
  struct LoopInfo {
    llvm::BasicBlock *continueDest; ///< Destination for continue statements
    llvm::BasicBlock *breakDest;    ///< Destination for break statements
  };

  /**
   * @brief Pushes a new loop onto the loop stack
   * @param continueDest Basic block to jump to for continue
   * @param breakDest Basic block to jump to for break
   */
  void pushLoop(llvm::BasicBlock *continueDest, llvm::BasicBlock *breakDest);

  /**
   * @brief Pops the current loop from the loop stack
   */
  void popLoop();

  /**
   * @brief Gets the current loop information
   * @return Pointer to current loop info, or nullptr if not in a loop
   */
  LoopInfo *getCurrentLoop();

  // Core components
  core::ErrorReporter &errorReporter_; ///< Error reporter for diagnostics
  CodeGenOptions options_;             ///< Code generation options
  LLVMContext context_;                ///< LLVM context manager
  LLVMTypeBuilder typeBuilder_;        ///< Type conversion utilities
  LLVMOptimizer optimizer_;            ///< Code optimization manager

  // Function generation state
  std::unique_ptr<LLVMFunction>
      currentFunction_;            ///< Current function being generated
  std::stack<LoopInfo> loopStack_; ///< Stack of nested loops

  // Namespace tracking
  std::vector<std::string> currentNamespace_; ///< Current namespace path

  /**
   * @brief Gets the current namespace prefix for symbol names
   * @return The namespace prefix string
   */
  std::string getCurrentNamespacePrefix() const;

  // Function lookup table
  std::unordered_map<std::string, llvm::Function *>
      functionTable_; ///< Function name to LLVM function mapping

  // Top-level statement collections
  std::vector<const nodes::AssemblyStmtNode *>
      topLevelAssemblyStatements_; ///< Assembly statements to execute in main
  std::vector<const nodes::ExpressionStmtNode *>
      topLevelExpressionStatements_; ///< Expression statements to execute in
                                     ///< main
};

} // namespace codegen