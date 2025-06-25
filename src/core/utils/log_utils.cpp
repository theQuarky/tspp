#include "core/utils/log_utils.h"
#include "ast_printer.h"
#include "lexer/patterns/token_maps.h"
#include <iomanip>

void printToken(const tokens::Token &token) {
  std::cout << "Token{type=" << static_cast<int>(token.getType())
            << ", category=\""
            << lexer::TokenMapUtils::getTokenCategory(token.getType()) << "\""
            << ", lexeme=\"" << token.getLexeme() << "\""
            << ", line=" << token.getLocation().getLine()
            << ", column=" << token.getLocation().getColumn();

  if (const auto &filename = token.getLocation().getFilename();
      !filename.empty()) {
    std::cout << ", file=\"" << filename << "\"";
  }

  if (token.getErrorMessage()) {
    std::cout << ", error=\"" << *token.getErrorMessage() << "\"";
  }

  std::cout << "}\n";
}

void printTokenStream(const std::vector<tokens::Token> &tokens) {
  std::cout << "Token Stream:\n" << std::string(80, '-') << "\n";

  for (size_t i = 0; i < tokens.size(); ++i) {
    std::cout << std::setw(4) << i << ": ";
    printToken(tokens[i]);
  }

  std::cout << std::string(80, '-') << "\n"
            << "Total tokens: " << tokens.size() << "\n";
}

void printAST(const parser::AST &ast) {
  core::ASTPrinter printer;
  printer.print(ast);
}

void printASTNode(const nodes::NodePtr &node, int indent) {
  core::ASTPrinter printer;
  printer.print(node);
}