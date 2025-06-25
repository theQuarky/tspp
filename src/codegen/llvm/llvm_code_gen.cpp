#include "codegen/llvm/llvm_code_gen.h"
#include "codegen/llvm/llvm_utils.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <iostream>
#include <regex>
#include <sstream>

namespace codegen {

LLVMCodeGen::LLVMCodeGen(core::ErrorReporter &errorReporter,
                         const std::string &moduleName)
    : errorReporter_(errorReporter), context_(moduleName),
      typeBuilder_(context_), optimizer_(context_) {}

bool LLVMCodeGen::generateCode(const parser::AST &ast) {
  try {
    auto &module = context_.getModule();

    // Clear any previous state
    topLevelAssemblyStatements_.clear();
    topLevelExpressionStatements_.clear();
    functionTable_.clear();

    // First, declare external functions that might be needed
    declareExternalFunctions();

    // Pre-pass: declare all types first
    declareTypes(ast);

    // Process all top-level declarations in the AST
    for (const auto &node : ast.getNodes()) {
      if (!visitTopLevelDeclaration(node)) {
        error(core::SourceLocation(),
              "Failed to process top-level declaration");
        return false;
      }
    }

    // Check if we have a main function, if not create one
    llvm::Function *mainFunc = module.getFunction("main");
    if (!mainFunc) {
      mainFunc = createDefaultMainFunction();
      if (!mainFunc) {
        error(core::SourceLocation(), "Failed to create main function");
        return false;
      }
    }

    // Verify all functions in the module
    for (auto &function : module) {
      if (llvm::verifyFunction(function, &llvm::errs())) {
        error(core::SourceLocation(),
              "Function verification failed for: " + function.getName().str());
        return false;
      }
    }

    // Apply optimizations if requested
    optimizer_.optimizeAll();

    return true;
  } catch (const std::exception &e) {
    error(core::SourceLocation(),
          std::string("Code generation failed: ") + e.what());
    return false;
  }
}

void LLVMCodeGen::declareExternalFunctions() {
  auto &module = context_.getModule();
  auto &llvmContext = context_.getContext();

  // Declare printf function for assembly statements
  if (!module.getFunction("printf")) {
    std::vector<llvm::Type *> printfArgs;
    printfArgs.push_back(llvm::Type::getInt8Ty(llvmContext));
    llvm::FunctionType *printfType =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(llvmContext), printfArgs,
                                true // varargs
        );
    llvm::Function::Create(printfType, llvm::Function::ExternalLinkage,
                           "printf", module);
  }

  // Declare puts function
  if (!module.getFunction("puts")) {
    std::vector<llvm::Type *> putsArgs;
    putsArgs.push_back(llvm::Type::getInt8Ty(llvmContext));
    llvm::FunctionType *putsType =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(llvmContext), putsArgs,
                                false // not varargs
        );
    llvm::Function::Create(putsType, llvm::Function::ExternalLinkage, "puts",
                           module);
  }

  // Declare malloc and free for dynamic allocation
  if (!module.getFunction("malloc")) {
    std::vector<llvm::Type *> mallocArgs;
    mallocArgs.push_back(llvm::Type::getInt64Ty(llvmContext)); // size_t
    llvm::FunctionType *mallocType = llvm::FunctionType::get(
        llvm::Type::getInt8Ty(llvmContext), mallocArgs, false);
    llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage,
                           "malloc", module);
  }

  if (!module.getFunction("free")) {
    std::vector<llvm::Type *> freeArgs;
    freeArgs.push_back(llvm::Type::getInt8Ty(llvmContext));
    llvm::FunctionType *freeType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(llvmContext), freeArgs, false);
    llvm::Function::Create(freeType, llvm::Function::ExternalLinkage, "free",
                           module);
  }
}

void LLVMCodeGen::declareTypes(const parser::AST &ast) {
  // Pre-pass to declare all types before generating code
  for (const auto &node : ast.getNodes()) {
    if (auto classDecl =
            std::dynamic_pointer_cast<nodes::ClassDeclNode>(node)) {
      // Declare class type
      std::vector<std::pair<std::string, llvm::Type *>> fields;
      // This would be populated from the class definition
      typeBuilder_.createStructType(classDecl->getName(), fields);
    }
    // Handle other type declarations as needed
  }
}

bool LLVMCodeGen::visitTopLevelDeclaration(const nodes::NodePtr &node) {
  try {
    if (auto funcDecl =
            std::dynamic_pointer_cast<nodes::FunctionDeclNode>(node)) {
      return visitFunctionDecl(funcDecl.get());
    } else if (auto varDecl =
                   std::dynamic_pointer_cast<nodes::VarDeclNode>(node)) {
      return visitGlobalVarDecl(varDecl.get());
    } else if (auto stmt =
                   std::dynamic_pointer_cast<nodes::StatementNode>(node)) {
      return visitTopLevelStatement(stmt.get());
    } else if (auto exprStmt =
                   std::dynamic_pointer_cast<nodes::ExpressionStmtNode>(node)) {
      return visitTopLevelStatement(exprStmt.get());
    } else if (auto classDecl =
                   std::dynamic_pointer_cast<nodes::ClassDeclNode>(node)) {
      visitClassDecl(classDecl.get());
      return true;
    } else if (auto namespaceDecl =
                   std::dynamic_pointer_cast<nodes::NamespaceDeclNode>(node)) {
      visitNamespaceDecl(namespaceDecl.get());
      return true;
    }

    // Unsupported node type - log warning but continue
    warning(core::SourceLocation(),
            "Unsupported top-level declaration type encountered");
    return true;
  } catch (const std::exception &e) {
    error(core::SourceLocation(),
          "Error processing top-level declaration: " + std::string(e.what()));
    return false;
  }
}

bool LLVMCodeGen::visitFunctionDecl(const nodes::FunctionDeclNode *node) {
  auto &module = context_.getModule();
  auto &llvmContext = context_.getContext();

  try {
    // Check if function already exists
    if (module.getFunction(node->getName())) {
      error(core::SourceLocation(),
            "Function '" + node->getName() + "' already declared");
      return false;
    }

    // Create function type
    std::vector<llvm::Type *> paramTypes;
    std::vector<std::string> paramNames;

    for (const auto &param : node->getParameters()) {
      // Use type builder for proper type conversion
      auto paramType = llvm::Type::getInt32Ty(llvmContext); // Simplified
      paramTypes.push_back(paramType);
      paramNames.push_back(param->getName());
    }

    // Determine return type
    llvm::Type *returnType = llvm::Type::getInt32Ty(llvmContext);
    if (node->getReturnType()) {
      // Use type builder for proper type conversion
      returnType = llvm::Type::getInt32Ty(llvmContext); // Simplified
    }

    llvm::FunctionType *functionType =
        llvm::FunctionType::get(returnType, paramTypes, false);

    llvm::Function *function = llvm::Function::Create(
        functionType, llvm::Function::ExternalLinkage, node->getName(), module);

    // Set parameter names
    unsigned idx = 0;
    for (auto &arg : function->args()) {
      if (idx < paramNames.size()) {
        arg.setName(paramNames[idx++]);
      }
    }

    // Store in function table
    functionTable_[node->getName()] = function;

    // Create function body if present
    if (node->getBody()) {
      llvm::BasicBlock *entryBlock =
          llvm::BasicBlock::Create(llvmContext, "entry", function);
      context_.getBuilder().SetInsertPoint(entryBlock);

      // Create function scope
      currentFunction_ =
          std::make_unique<LLVMFunction>(context_, function, nullptr);

      // Map parameters to local variables
      currentFunction_->mapParameters(paramNames);

      // Visit function body
      visitBlock(node->getBody().get());

      // Ensure function has a return statement
      llvm::BasicBlock *currentBlock = context_.getBuilder().GetInsertBlock();
      if (currentBlock && !currentBlock->getTerminator()) {
        if (returnType->isVoidTy()) {
          context_.getBuilder().CreateRetVoid();
        } else {
          // Return default value for the type
          llvm::Value *defaultValue = llvm::Constant::getNullValue(returnType);
          context_.getBuilder().CreateRet(defaultValue);
        }
      }

      currentFunction_.reset();
    }

    return true;
  } catch (const std::exception &e) {
    error(core::SourceLocation(), "Error in function declaration '" +
                                      node->getName() + "': " + e.what());
    return false;
  }
}

bool LLVMCodeGen::visitGlobalVarDecl(const nodes::VarDeclNode *node) {
  auto &module = context_.getModule();
  auto &llvmContext = context_.getContext();

  try {
    // Check if global variable already exists
    if (module.getGlobalVariable(node->getName())) {
      error(core::SourceLocation(),
            "Global variable '" + node->getName() + "' already declared");
      return false;
    }

    // Determine variable type
    llvm::Type *varType = llvm::Type::getInt32Ty(llvmContext); // Simplified

    llvm::Constant *initializer = llvm::Constant::getNullValue(varType);

    // Handle initializer if present
    if (node->getInitializer()) {
      // Create a temporary function to evaluate the initializer
      llvm::Function *tempFunc = llvm::Function::Create(
          llvm::FunctionType::get(llvm::Type::getVoidTy(llvmContext), false),
          llvm::Function::PrivateLinkage, "__temp_init", module);

      llvm::BasicBlock *tempBlock =
          llvm::BasicBlock::Create(llvmContext, "entry", tempFunc);
      context_.getBuilder().SetInsertPoint(tempBlock);

      LLVMValue initValue = visitExpr(node->getInitializer().get());
      if (initValue.isValid()) {
        if (auto constant =
                llvm::dyn_cast<llvm::Constant>(initValue.getValue())) {
          initializer = constant;
        }
      }

      // Clean up temporary function
      tempFunc->eraseFromParent();
    }

    llvm::GlobalVariable *globalVar = new llvm::GlobalVariable(
        module, varType, node->isConst(), llvm::GlobalValue::ExternalLinkage,
        initializer, node->getName());

    return true;
  } catch (const std::exception &e) {
    error(core::SourceLocation(), "Error in global variable declaration '" +
                                      node->getName() + "': " + e.what());
    return false;
  }
}

bool LLVMCodeGen::visitTopLevelStatement(const nodes::StatementNode *node) {
  try {
    // Store top-level statements to be executed in main
    if (auto asmStmt = dynamic_cast<const nodes::AssemblyStmtNode *>(node)) {
      topLevelAssemblyStatements_.push_back(asmStmt);
      return true;
    } else if (auto exprStmt =
                   dynamic_cast<const nodes::ExpressionStmtNode *>(node)) {
      topLevelExpressionStatements_.push_back(exprStmt);
      return true;
    }

    // For other statement types, we might want to handle them differently
    warning(core::SourceLocation(), "Unsupported top-level statement type");
    return true;
  } catch (const std::exception &e) {
    error(core::SourceLocation(),
          "Error processing top-level statement: " + std::string(e.what()));
    return false;
  }
}

llvm::Function *LLVMCodeGen::createDefaultMainFunction() {
  auto &module = context_.getModule();
  auto &llvmContext = context_.getContext();
  auto &builder = context_.getBuilder();

  try {
    // Create main function
    llvm::FunctionType *mainType =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(llvmContext), false);

    llvm::Function *mainFunc = llvm::Function::Create(
        mainType, llvm::Function::ExternalLinkage, "main", module);

    llvm::BasicBlock *entry =
        llvm::BasicBlock::Create(llvmContext, "entry", mainFunc);
    builder.SetInsertPoint(entry);

    // Execute any top-level assembly statements
    for (const auto &asmStmt : topLevelAssemblyStatements_) {
      if (!visitAssemblyStatement(asmStmt)) {
        warning(core::SourceLocation(), "Failed to process assembly statement");
      }
    }

    // Execute any top-level expressions
    for (const auto &exprStmt : topLevelExpressionStatements_) {
      if (!visitExpressionStatement(exprStmt)) {
        warning(core::SourceLocation(),
                "Failed to process expression statement");
      }
    }

    // Return 0
    builder.CreateRet(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), 0, true));

    return mainFunc;
  } catch (const std::exception &e) {
    error(core::SourceLocation(),
          "Error creating main function: " + std::string(e.what()));
    return nullptr;
  }
}

bool LLVMCodeGen::visitAssemblyStatement(const nodes::AssemblyStmtNode *node) {
  auto &builder = context_.getBuilder();
  auto &llvmContext = context_.getContext();
  auto &module = context_.getModule();

  try {
    std::string asmCode = node->getCode();

    // Handle printf calls specially
    if (isSimplePrintfCall(asmCode)) {
      llvm::Function *printfFunc = module.getFunction("printf");

      if (printfFunc) {
        // Extract the string from printf("string")
        std::string printStr = extractPrintfString(asmCode);

        // Create the string constant
        llvm::Value *strConstant =
            LLVMUtils::createGlobalString(context_, printStr, "printf_str");

        // Call printf
        builder.CreateCall(printfFunc, {strConstant});
        return true;
      } else {
        error(core::SourceLocation(), "printf function not available");
        return false;
      }
    }

    // For other assembly code, create inline assembly
    std::vector<llvm::Type *> asmParamTypes;
    llvm::FunctionType *asmType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(llvmContext), asmParamTypes, false);

    llvm::InlineAsm *inlineAsm =
        llvm::InlineAsm::get(asmType, asmCode, "", true, false);

    builder.CreateCall(inlineAsm);
    return true;
  } catch (const std::exception &e) {
    error(core::SourceLocation(),
          "Error processing assembly statement: " + std::string(e.what()));
    return false;
  }
}

bool LLVMCodeGen::visitExpressionStatement(
    const nodes::ExpressionStmtNode *node) {
  try {
    // Visit the expression (this might have side effects)
    if (node->getExpression()) {
      LLVMValue result = visitExpr(node->getExpression().get());
      return result.isValid() || true; // Allow invalid results for statements
    }
    return true;
  } catch (const std::exception &e) {
    error(core::SourceLocation(),
          "Error processing expression statement: " + std::string(e.what()));
    return false;
  }
}

// String parsing helpers
std::string LLVMCodeGen::parseStringLiteral(const std::string &input) {
  std::string result = input;

  // Handle escape sequences
  std::regex newlineRegex(R"(\\n)");
  result = std::regex_replace(result, newlineRegex, "\n");

  std::regex tabRegex(R"(\\t)");
  result = std::regex_replace(result, tabRegex, "\t");

  std::regex backslashRegex(R"(\\\\)");
  result = std::regex_replace(result, backslashRegex, "\\");

  std::regex quoteRegex(R"(\\")");
  result = std::regex_replace(result, quoteRegex, "\"");

  return result;
}

bool LLVMCodeGen::isSimplePrintfCall(const std::string &asmCode) {
  // Check if it's a simple printf call: printf("...")
  std::regex printfRegex(R"(printf\s*\(\s*\".*\"\s*\))");
  return std::regex_match(asmCode, printfRegex);
}

std::string LLVMCodeGen::extractPrintfString(const std::string &asmCode) {
  // Extract string from printf("string")
  std::regex stringRegex(R"(printf\s*\(\s*\"(.*)\"\s*\))");
  std::smatch match;

  if (std::regex_search(asmCode, match, stringRegex)) {
    return parseStringLiteral(match[1].str());
  }

  return "";
}

// Utility methods for loop management
void LLVMCodeGen::pushLoop(llvm::BasicBlock *continueDest,
                           llvm::BasicBlock *breakDest) {
  loopStack_.push({continueDest, breakDest});
}

void LLVMCodeGen::popLoop() {
  if (!loopStack_.empty()) {
    loopStack_.pop();
  }
}

LLVMCodeGen::LoopInfo *LLVMCodeGen::getCurrentLoop() {
  if (loopStack_.empty()) {
    return nullptr;
  }
  return &loopStack_.top();
}

std::string LLVMCodeGen::getCurrentNamespacePrefix() const {
  if (currentNamespace_.empty()) {
    return "";
  }

  std::stringstream ss;
  for (size_t i = 0; i < currentNamespace_.size(); ++i) {
    if (i > 0)
      ss << "::";
    ss << currentNamespace_[i];
  }
  ss << "::";
  return ss.str();
}

// Core visitor implementations
LLVMValue LLVMCodeGen::visitVarDecl(const nodes::VarDeclNode *node,
                                    bool isGlobal) {
  if (isGlobal) {
    visitGlobalVarDecl(node);
    return LLVMValue();
  }

  // Local variable declaration
  auto &builder = context_.getBuilder();
  auto &llvmContext = context_.getContext();

  // Determine variable type (simplified)
  llvm::Type *varType = llvm::Type::getInt32Ty(llvmContext);

  // Create allocation
  llvm::AllocaInst *alloca =
      builder.CreateAlloca(varType, nullptr, node->getName());

  // Handle initializer
  if (node->getInitializer()) {
    LLVMValue initValue = visitExpr(node->getInitializer().get());
    if (initValue.isValid()) {
      llvm::Value *loaded = initValue.loadIfLValue(builder).getValue();
      builder.CreateStore(loaded, alloca);
    }
  }

  // Register variable in current scope
  if (currentFunction_) {
    LLVMValue varValue(alloca, nullptr, true); // It's an lvalue
    currentFunction_->declareVariable(node->getName(), varValue);
  }

  return LLVMValue(alloca, nullptr, true);
}

LLVMValue LLVMCodeGen::visitBlock(const nodes::BlockNode *node) {
  // Enter new scope
  if (currentFunction_) {
    currentFunction_->enterScope();
  }

  LLVMValue lastValue;
  for (const auto &stmt : node->getStatements()) {
    lastValue = visitStmt(stmt.get());
  }

  // Exit scope
  if (currentFunction_) {
    currentFunction_->exitScope();
  }

  return lastValue;
}

LLVMValue LLVMCodeGen::visitStmt(const nodes::StatementNode *node) {
  if (auto exprStmt = dynamic_cast<const nodes::ExpressionStmtNode *>(node)) {
    return visitExprStmt(exprStmt);
  } else if (auto blockStmt = dynamic_cast<const nodes::BlockNode *>(node)) {
    return visitBlock(blockStmt);
  } else if (auto asmStmt =
                 dynamic_cast<const nodes::AssemblyStmtNode *>(node)) {
    visitAssemblyStatement(asmStmt);
    return LLVMValue();
  } else if (auto declStmt =
                 dynamic_cast<const nodes::DeclarationStmtNode *>(node)) {
    return visitDeclStmt(declStmt);
  } else if (auto returnStmt =
                 dynamic_cast<const nodes::ReturnStmtNode *>(node)) {
    return visitReturnStmt(returnStmt);
  }

  // Add other statement types as needed
  return LLVMValue();
}

LLVMValue LLVMCodeGen::visitExprStmt(const nodes::ExpressionStmtNode *node) {
  if (node->getExpression()) {
    return visitExpr(node->getExpression().get());
  }
  return LLVMValue();
}

LLVMValue LLVMCodeGen::visitExpr(const nodes::ExpressionNode *node) {
  if (auto literal = dynamic_cast<const nodes::LiteralExpressionNode *>(node)) {
    return visitLiteralExpr(literal);
  } else if (auto identifier =
                 dynamic_cast<const nodes::IdentifierExpressionNode *>(node)) {
    return visitIdentifierExpr(identifier);
  } else if (auto binary =
                 dynamic_cast<const nodes::BinaryExpressionNode *>(node)) {
    return visitBinaryExpr(binary);
  } else if (auto call =
                 dynamic_cast<const nodes::CallExpressionNode *>(node)) {
    return visitCallExpr(call);
  }

  // Add other expression types as needed
  return LLVMValue();
}

LLVMValue
LLVMCodeGen::visitLiteralExpr(const nodes::LiteralExpressionNode *node) {
  auto &llvmContext = context_.getContext();

  switch (node->getExpressionType()) {
  case tokens::TokenType::NUMBER: {
    try {
      int value = std::stoi(node->getValue());
      auto intType = llvm::Type::getInt32Ty(llvmContext);
      auto intValue = llvm::ConstantInt::get(intType, value);
      return LLVMValue(intValue, nullptr);
    } catch (const std::exception &e) {
      error(core::SourceLocation(),
            "Invalid number literal: " + node->getValue());
      return LLVMValue();
    }
  }
  case tokens::TokenType::STRING_LITERAL: {
    auto strValue = LLVMUtils::createGlobalString(context_, node->getValue());
    return LLVMValue(strValue, nullptr);
  }
  case tokens::TokenType::TRUE:
  case tokens::TokenType::FALSE: {
    bool value = (node->getExpressionType() == tokens::TokenType::TRUE);
    auto boolValue =
        llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvmContext), value);
    return LLVMValue(boolValue, nullptr);
  }
  default:
    error(core::SourceLocation(), "Unsupported literal type");
    return LLVMValue();
  }
}

// Optimization and file writing methods
void LLVMCodeGen::optimize(OptimizationLevel level) {
  optimizer_.setOptimizationLevel(level);
  optimizer_.optimizeAll();
}

bool LLVMCodeGen::writeToFile(const std::string &filename) const {
  return context_.writeModuleToFile(filename);
}

bool LLVMCodeGen::executeCode() {
  try {
    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Clone the module since ExecutionEngine takes ownership
    auto moduleClone = llvm::CloneModule(context_.getModule());
    if (!moduleClone) {
      error(core::SourceLocation(), "Failed to clone module for execution");
      return false;
    }

    // Create execution engine with proper error handling
    std::string errorStr;
    llvm::ExecutionEngine *executionEngine =
        llvm::EngineBuilder(std::move(moduleClone))
            .setErrorStr(&errorStr)
            .setEngineKind(llvm::EngineKind::JIT)
            .create();

    if (!executionEngine) {
      error(core::SourceLocation(),
            "Failed to create execution engine: " + errorStr);
      return false;
    }

    // Use unique_ptr for automatic cleanup
    std::unique_ptr<llvm::ExecutionEngine> enginePtr(executionEngine);

    // Find the main function
    llvm::Function *mainFunc = enginePtr->FindFunctionNamed("main");
    if (!mainFunc) {
      error(core::SourceLocation(), "No main function found for execution");
      return false;
    }

    // Finalize the engine (important for MCJIT)
    enginePtr->finalizeObject();

    // Prepare arguments (none for main in this case)
    std::vector<llvm::GenericValue> args;

    // Execute the function
    llvm::GenericValue result = enginePtr->runFunction(mainFunc, args);

    // Print the return value
    std::cout << "Program executed, returned: " << result.IntVal.getSExtValue()
              << std::endl;

    return true;
  } catch (const std::exception &e) {
    error(core::SourceLocation(),
          std::string("Error during execution: ") + e.what());
    return false;
  }
}

// Error reporting
void LLVMCodeGen::error(const core::SourceLocation &location,
                        const std::string &message) {
  errorReporter_.error(location, message);
}

void LLVMCodeGen::warning(const core::SourceLocation &location,
                          const std::string &message) {
  errorReporter_.warning(location, message);
}

// Stub implementations for other required methods
void LLVMCodeGen::visitGlobalDecl(const nodes::NodePtr &node) {}
void LLVMCodeGen::visitClassDecl(const nodes::ClassDeclNode *node) {}
void LLVMCodeGen::visitNamespaceDecl(const nodes::NamespaceDeclNode *node) {}
void LLVMCodeGen::visitEnumDecl(const nodes::EnumDeclNode *node) {}
void LLVMCodeGen::visitInterfaceDecl(const nodes::InterfaceDeclNode *node) {}
LLVMValue LLVMCodeGen::visitParameter(const nodes::ParameterNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitIfStmt(const nodes::IfStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitWhileStmt(const nodes::WhileStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitDoWhileStmt(const nodes::DoWhileStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitForStmt(const nodes::ForStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitForOfStmt(const nodes::ForOfStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitReturnStmt(const nodes::ReturnStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitBreakStmt(const nodes::BreakStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitContinueStmt(const nodes::ContinueStmtNode *node) {
  auto &builder = context_.getBuilder();

  LoopInfo *currentLoop = getCurrentLoop();
  if (!currentLoop) {
    error(core::SourceLocation(), "Continue statement outside of loop");
    return LLVMValue();
  }

  builder.CreateBr(currentLoop->continueDest);
  return LLVMValue();
}

LLVMValue LLVMCodeGen::visitSwitchStmt(const nodes::SwitchStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitTryStmt(const nodes::TryStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitThrowStmt(const nodes::ThrowStmtNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitDeclStmt(const nodes::DeclarationStmtNode *node) {
  // Handle variable declarations within statements
  if (auto varDecl = node->getDeclaration()) {
    if (auto varDeclNode =
            std::dynamic_pointer_cast<nodes::VarDeclNode>(varDecl)) {
      return visitVarDecl(varDeclNode.get(), false);
    }
  }
  return LLVMValue();
}

LLVMValue
LLVMCodeGen::visitBinaryExpr(const nodes::BinaryExpressionNode *node) {
  auto &builder = context_.getBuilder();
  auto &llvmContext = context_.getContext();

  LLVMValue left = visitExpr(node->getLeft().get());
  LLVMValue right = visitExpr(node->getRight().get());

  if (!left.isValid() || !right.isValid()) {
    error(core::SourceLocation(), "Invalid operands in binary expression");
    return LLVMValue();
  }

  // Load values if they are lvalues
  llvm::Value *leftVal = left.loadIfLValue(builder).getValue();
  llvm::Value *rightVal = right.loadIfLValue(builder).getValue();

  llvm::Value *result = nullptr;

  switch (node->getExpressionType()) {
  case tokens::TokenType::PLUS:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateAdd(leftVal, rightVal, "add");
    } else if (leftVal->getType()->isFloatingPointTy() ||
               rightVal->getType()->isFloatingPointTy()) {
      // Handle float addition (would need proper type conversion)
      result = builder.CreateFAdd(leftVal, rightVal, "fadd");
    }
    break;

  case tokens::TokenType::MINUS:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateSub(leftVal, rightVal, "sub");
    } else {
      result = builder.CreateFSub(leftVal, rightVal, "fsub");
    }
    break;

  case tokens::TokenType::STAR:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateMul(leftVal, rightVal, "mul");
    } else {
      result = builder.CreateFMul(leftVal, rightVal, "fmul");
    }
    break;

  case tokens::TokenType::SLASH:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateSDiv(leftVal, rightVal, "div");
    } else {
      result = builder.CreateFDiv(leftVal, rightVal, "fdiv");
    }
    break;

  case tokens::TokenType::PERCENT:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateSRem(leftVal, rightVal, "mod");
    }
    break;

  case tokens::TokenType::EQUALS_EQUALS:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateICmpEQ(leftVal, rightVal, "eq");
    } else {
      result = builder.CreateFCmpOEQ(leftVal, rightVal, "feq");
    }
    break;

  case tokens::TokenType::EXCLAIM_EQUALS:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateICmpNE(leftVal, rightVal, "ne");
    } else {
      result = builder.CreateFCmpONE(leftVal, rightVal, "fne");
    }
    break;

  case tokens::TokenType::LEFT_BRACKET:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateICmpSLT(leftVal, rightVal, "lt");
    } else {
      result = builder.CreateFCmpOLT(leftVal, rightVal, "flt");
    }
    break;

  case tokens::TokenType::RIGHT_BRACKET:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateICmpSGT(leftVal, rightVal, "gt");
    } else {
      result = builder.CreateFCmpOGT(leftVal, rightVal, "fgt");
    }
    break;

  case tokens::TokenType::LESS_EQUALS:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateICmpSLE(leftVal, rightVal, "le");
    } else {
      result = builder.CreateFCmpOLE(leftVal, rightVal, "fle");
    }
    break;

  case tokens::TokenType::GREATER_EQUALS:
    if (leftVal->getType()->isIntegerTy() &&
        rightVal->getType()->isIntegerTy()) {
      result = builder.CreateICmpSGE(leftVal, rightVal, "ge");
    } else {
      result = builder.CreateFCmpOGE(leftVal, rightVal, "fge");
    }
    break;

    // case tokens::TokenType::LOGICAL_AND:
    //   result = builder.CreateAnd(leftVal, rightVal, "and");
    //   break;

    // case tokens::TokenType::LOGICAL_OR:
    //   result = builder.CreateOr(leftVal, rightVal, "or");
    //   break;

  default:
    error(core::SourceLocation(), "Unsupported binary operator");
    return LLVMValue();
  }

  if (!result) {
    error(core::SourceLocation(), "Failed to generate binary operation");
    return LLVMValue();
  }

  return LLVMValue(result, nullptr);
}

LLVMValue LLVMCodeGen::visitUnaryExpr(const nodes::UnaryExpressionNode *node) {
  auto &builder = context_.getBuilder();
  auto &llvmContext = context_.getContext();

  LLVMValue operand = visitExpr(node->getOperand().get());
  if (!operand.isValid()) {
    error(core::SourceLocation(), "Invalid operand in unary expression");
    return LLVMValue();
  }

  llvm::Value *operandVal = operand.loadIfLValue(builder).getValue();
  llvm::Value *result = nullptr;

  switch (node->getOperand()->getExpressionType()) {
  case tokens::TokenType::MINUS:
    if (operandVal->getType()->isIntegerTy()) {
      result = builder.CreateNeg(operandVal, "neg");
    } else {
      result = builder.CreateFNeg(operandVal, "fneg");
    }
    break;

  case tokens::TokenType::PLUS:
    // Unary plus is essentially a no-op
    result = operandVal;
    break;

  case tokens::TokenType::EXCLAIM:
    result = builder.CreateNot(operandVal, "not");
    break;

  case tokens::TokenType::PLUS_PLUS:
    if (operand.isLValue()) {
      llvm::Value *one = llvm::ConstantInt::get(operandVal->getType(), 1);
      result = builder.CreateAdd(operandVal, one, "inc");
      builder.CreateStore(result, operand.getValue());
      return operand; // Return the lvalue for pre-increment
    }
    break;

  case tokens::TokenType::MINUS_MINUS:
    if (operand.isLValue()) {
      llvm::Value *one = llvm::ConstantInt::get(operandVal->getType(), 1);
      result = builder.CreateSub(operandVal, one, "dec");
      builder.CreateStore(result, operand.getValue());
      return operand; // Return the lvalue for pre-decrement
    }
    break;

  default:
    error(core::SourceLocation(), "Unsupported unary operator");
    return LLVMValue();
  }

  if (!result) {
    error(core::SourceLocation(), "Failed to generate unary operation");
    return LLVMValue();
  }

  return LLVMValue(result, nullptr);
}

LLVMValue
LLVMCodeGen::visitIdentifierExpr(const nodes::IdentifierExpressionNode *node) {
  std::string name = node->getName();

  // First, look for local variables
  if (currentFunction_) {
    LLVMValue var = currentFunction_->getVariable(name);
    if (var.isValid()) {
      return var;
    }
  }

  // Then look for global variables
  auto &module = context_.getModule();
  if (llvm::GlobalVariable *globalVar = module.getGlobalVariable(name)) {
    return LLVMValue(globalVar, nullptr, true); // Global variables are lvalues
  }

  // Look for functions
  if (llvm::Function *func = module.getFunction(name)) {
    return LLVMValue(func, nullptr);
  }

  error(core::SourceLocation(), "Undefined identifier: " + name);
  return LLVMValue();
}

LLVMValue LLVMCodeGen::visitCallExpr(const nodes::CallExpressionNode *node) {
  auto &builder = context_.getBuilder();
  auto &module = context_.getModule();

  // Get the function to call
  std::string funcName;
  if (auto identExpr = dynamic_cast<const nodes::IdentifierExpressionNode *>(
          node->getCallee().get())) {
    funcName = identExpr->getName();
  } else {
    error(core::SourceLocation(), "Complex function calls not yet supported");
    return LLVMValue();
  }

  llvm::Function *function = module.getFunction(funcName);
  if (!function) {
    error(core::SourceLocation(), "Function not found: " + funcName);
    return LLVMValue();
  }

  // Evaluate arguments
  std::vector<llvm::Value *> args;
  for (const auto &arg : node->getArguments()) {
    LLVMValue argValue = visitExpr(arg.get());
    if (!argValue.isValid()) {
      error(core::SourceLocation(), "Invalid argument in function call");
      return LLVMValue();
    }

    // Load the value if it's an lvalue
    llvm::Value *loadedArg = argValue.loadIfLValue(builder).getValue();
    args.push_back(loadedArg);
  }

  // Check argument count
  if (args.size() != function->arg_size()) {
    error(core::SourceLocation(), "Argument count mismatch for function " +
                                      funcName + ": expected " +
                                      std::to_string(function->arg_size()) +
                                      ", got " + std::to_string(args.size()));
    return LLVMValue();
  }

  // Create the call
  llvm::Value *result = builder.CreateCall(function, args, "call");
  return LLVMValue(result, nullptr);
}

LLVMValue
LLVMCodeGen::visitMemberExpr(const nodes::MemberExpressionNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitIndexExpr(const nodes::IndexExpressionNode *node) {
  return LLVMValue();
}

LLVMValue
LLVMCodeGen::visitAssignmentExpr(const nodes::AssignmentExpressionNode *node) {
  auto &builder = context_.getBuilder();

  // Get the left-hand side (must be an lvalue)
  LLVMValue lhs = visitExpr(node->getValue().get());
  if (!lhs.isValid() || !lhs.isLValue()) {
    error(core::SourceLocation(),
          "Left-hand side of assignment must be an lvalue");
    return LLVMValue();
  }

  // Get the right-hand side
  LLVMValue rhs = visitExpr(node->getTarget().get());
  if (!rhs.isValid()) {
    error(core::SourceLocation(), "Invalid right-hand side in assignment");
    return LLVMValue();
  }

  // Load the right-hand side value
  llvm::Value *rhsValue = rhs.loadIfLValue(builder).getValue();

  // Store the value
  builder.CreateStore(rhsValue, lhs.getValue());

  // Return the lvalue
  return lhs;
}

LLVMValue LLVMCodeGen::visitNewExpr(const nodes::NewExpressionNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitCastExpr(const nodes::CastExpressionNode *node) {
  return LLVMValue();
}
LLVMValue LLVMCodeGen::visitArrayLiteral(const nodes::ArrayLiteralNode *node) {
  return LLVMValue();
}

} // namespace codegen