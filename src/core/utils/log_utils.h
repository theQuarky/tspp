#pragma once
#include "tokens/tokens.h"
#include "parser/ast.h"
#include <vector>

/**
 * @brief Prints details of a single token to the console
 * @param token Token to print
 * @throws None
 */
void printToken(const tokens::Token &token);

/**
 * @brief Prints the entire token stream to the console
 * @param tokens Token vector to print
 * @throws None
 */
void printTokenStream(const std::vector<tokens::Token> &tokens);

/**
 * @brief Prints AST node with indentation and detailed info
 * @param node AST node to print
 * @param indent Current indentation level
 */
void printASTNode(const nodes::NodePtr& node, int indent = 0);

/**
 * @brief Prints the entire AST structure
 * @param ast Abstract Syntax Tree to print
 */
void printAST(const parser::AST& ast);