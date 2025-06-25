#pragma once
#include "parser/ast.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/nodes/expression_nodes.h"
#include "parser/nodes/statement_nodes.h"
#include "parser/nodes/type_nodes.h"
#include "tokens/token_type.h"
#include <functional>
#include <iostream>
#include <ostream>
#include <string>

namespace core {

class ASTPrinter {
private:
  // ANSI color codes for terminal output
  static constexpr const char *RESET = "\033[0m";
  static constexpr const char *RED = "\033[31m";
  static constexpr const char *GREEN = "\033[32m";
  static constexpr const char *YELLOW = "\033[33m";
  static constexpr const char *BLUE = "\033[34m";

  // Current indentation level
  int indentLevel_ = 0;

  // Helper: Print indentation spaces based on indentLevel_
  void indent() const { std::cout << std::string(indentLevel_ * 2, ' '); }

  // Helper: Print a line with a label and optional color, and a newline.
  void printLine(const std::string &label, const char *color = RESET) {
    indent();
    std::cout << color << label << RESET << "\n";
  }

  // Helper: Get source location as a string "(line:column)".
  std::string getLocationString(const core::SourceLocation &loc) const {
    return "(" + std::to_string(loc.getLine()) + ":" +
           std::to_string(loc.getColumn()) + ")";
  }

  // Helper: A lambda-style function to manage indent scope.
  void withIndent(const std::function<void()> &func) {
    indentLevel_++;
    func();
    indentLevel_--;
  }

  //---------------------------------------------------------------------------
  // Visitor Functions for Declaration Nodes
  //---------------------------------------------------------------------------

  void visitClassDecl(const nodes::ClassDeclNode *node) {
    printLine("ClassDecl " + getLocationString(node->getLocation()), BLUE);
    withIndent([&]() {
      printLine("Name: '" + node->getName() + "'");
      const auto &classModifiers = node->getClassModifiers();
      if (!classModifiers.empty()) {
        printLine("Class Modifiers:");
        withIndent([&]() {
          for (auto mod : classModifiers) {
            printLine(tokenTypeToString(mod));
          }
        });
      }
      if (auto genericClass =
              dynamic_cast<const nodes::GenericClassDeclNode *>(node)) {
        if (!genericClass->getGenericParams().empty()) {
          printLine("Generic Parameters:");
          withIndent([&]() {
            for (const auto &param : genericClass->getGenericParams())
              printLine(param->toString());
          });
        }
        // if (!genericClass->getConstraints().empty()) {
        //   printLine("Constraints:");
        //   withIndent([&]() {
        //     for (const auto &[paramName, constraint] :
        //          genericFunc->getConstraints())
        //       printLine(paramName + ": " + constraint->toString());
        //   });
        // }
      }

      if (node->getBaseClass()) {
        printLine("Base Class:");
        withIndent([&]() { visitType(node->getBaseClass().get()); });
      }
      const auto &interfaces = node->getInterfaces();
      if (!interfaces.empty()) {
        printLine("Interfaces:");
        withIndent([&]() {
          for (const auto &iface : interfaces)
            visitType(iface.get());
        });
      }
      const auto &members = node->getMembers();
      if (!members.empty()) {
        printLine("Members:");
        withIndent([&]() {
          for (const auto &member : members)
            print(member); // Dispatch to correct visitor method.
        });
      }
    });
  }

  void visitMethodDecl(const nodes::MethodDeclNode *node) {
    printLine("MethodDecl " + getLocationString(node->getLocation()), BLUE);
    withIndent([&]() {
      printLine("Access: " + tokenTypeToString(node->getAccessModifier()));
      printLine("Name: '" + node->getName() + "'");
      const auto &parameters = node->getParameters();
      if (!parameters.empty()) {
        printLine("Parameters:");
        withIndent([&]() {
          for (const auto &param : parameters)
            visitParameter(param.get());
        });
      }
      if (node->getReturnType()) {
        printLine("Return Type:");
        withIndent([&]() { visitType(node->getReturnType().get()); });
      }
      if (!node->getThrowsTypes().empty()) {
        printLine("Throws:");
        withIndent([&]() {
          for (const auto &throwT : node->getThrowsTypes())
            visitType(throwT.get());
        });
      }
      if (!node->getModifiers().empty()) {
        printLine("Method Modifiers:");
        withIndent([&]() {
          for (auto mod : node->getModifiers())
            printLine(modifierToString(mod));
        });
      }
      if (node->getBody()) {
        printLine("Body:");
        withIndent([&]() { visitBlock(node->getBody().get()); });
      }
    });
  }

  void visitConstructorDecl(const nodes::ConstructorDeclNode *node) {
    printLine("ConstructorDecl " + getLocationString(node->getLocation()),
              BLUE);
    withIndent([&]() {
      printLine("Access: " + tokenTypeToString(node->getAccessModifier()));
      const auto &params = node->getParameters();
      if (!params.empty()) {
        printLine("Parameters:");
        withIndent([&]() {
          for (const auto &param : params)
            visitParameter(param.get());
        });
      }
      if (node->getBody()) {
        printLine("Body:");
        withIndent([&]() { visitBlock(node->getBody().get()); });
      }
    });
  }

  void visitFieldDecl(const nodes::FieldDeclNode *node) {
    printLine("FieldDecl " + getLocationString(node->getLocation()), GREEN);
    withIndent([&]() {
      printLine("Access: " + tokenTypeToString(node->getAccessModifier()));
      if (node->isConst())
        printLine("Const: true");
      printLine("Name: '" + node->getName() + "'");
      if (node->getType()) {
        printLine("Type:");
        withIndent([&]() { visitType(node->getType().get()); });
      }
      if (node->getInitializer()) {
        printLine("Initializer:");
        withIndent([&]() { visitExpr(node->getInitializer().get()); });
      }
    });
  }

  void visitFuncDecl(const nodes::FunctionDeclNode *node) {
    printLine("FunctionDecl " + getLocationString(node->getLocation()), BLUE);
    withIndent([&]() {
      const auto &modifiers = node->getModifiers();
      if (!modifiers.empty()) {
        printLine("Modifiers:");
        withIndent([&]() {
          for (const auto &modifier : modifiers)
            printLine(modifierToString(modifier));
        });
      }
      printLine("Name: '" + node->getName() + "'");
      if (auto genericFunc =
              dynamic_cast<const nodes::GenericFunctionDeclNode *>(node)) {
        if (!genericFunc->getGenericParams().empty()) {
          printLine("Generic Parameters:");
          withIndent([&]() {
            for (const auto &param : genericFunc->getGenericParams())
              printLine(param->toString());
          });
        }
        if (!genericFunc->getConstraints().empty()) {
          printLine("Constraints:");
          withIndent([&]() {
            for (const auto &[paramName, constraint] :
                 genericFunc->getConstraints())
              printLine(paramName + ": " + constraint->toString());
          });
        }
      }
      printLine("Parameters:");
      withIndent([&]() {
        for (const auto &param : node->getParameters())
          visitParameter(param.get());
      });
      if (node->getReturnType()) {
        printLine("Return Type:");
        withIndent([&]() { visitType(node->getReturnType().get()); });
      }
      if (!node->getThrowsTypes().empty()) {
        printLine("Throws:");
        withIndent([&]() {
          for (const auto &throwType : node->getThrowsTypes())
            visitType(throwType.get());
        });
      }
      if (node->getBody()) {
        printLine("Body:");
        withIndent([&]() { visitBlock(node->getBody().get()); });
      }
      if (node->isAsync())
        printLine("Async: true");
    });
  }

  void visitVarDecl(const nodes::VarDeclNode *node) {
    printLine("VarDecl", GREEN);
    withIndent([&]() {
      printLine("Name: '" + node->getName() + "' " +
                getLocationString(node->getLocation()));
      indent();
      std::cout << "Storage: ";
      switch (node->getStorageClass()) {
      case tokens::TokenType::HEAP:
        std::cout << "#heap";
        break;
      case tokens::TokenType::STACK:
        std::cout << "#stack";
        break;
      case tokens::TokenType::STATIC:
        std::cout << "#static";
        break;
      default:
        std::cout << "none";
        break;
      }
      std::cout << "\n";
      if (node->isConst())
        printLine("Qualifier: const");
      if (node->getType()) {
        printLine("Type:");
        withIndent([&]() { visitType(node->getType().get()); });
      }
      const auto &attributes = node->getAttributes();
      if (!attributes.empty()) {
        printLine("Attributes:");
        withIndent([&]() {
          for (const auto &attr : attributes)
            visitAttribute(attr.get());
        });
      }
      if (node->getInitializer()) {
        printLine("Initializer:");
        withIndent([&]() { visitExpr(node->getInitializer().get()); });
      }
    });
  }

  void visitEnumDecl(const nodes::EnumDeclNode *node) {
    printLine("EnumDecl " + getLocationString(node->getLocation()), BLUE);
    withIndent([&]() {
      printLine("Name: '" + node->getName() + "'");

      // Print underlying type if present
      if (node->getUnderlyingType()) {
        printLine("Underlying Type:");
        withIndent([&]() { visitType(node->getUnderlyingType().get()); });
      }

      // Print enum members
      const auto &members = node->getMembers();
      if (!members.empty()) {
        printLine("Members:");
        withIndent([&]() {
          for (const auto &member : members) {
            visitEnumMember(member.get());
          }
        });
      }
    });
  }

  void visitEnumMember(const nodes::EnumMemberNode *node) {
    printLine("EnumMember '" + node->getName() + "' " +
                  getLocationString(node->getLocation()),
              YELLOW);

    // Print value if present
    if (node->getValue()) {
      withIndent([&]() {
        printLine("Value:");
        withIndent([&]() { visitExpr(node->getValue().get()); });
      });
    }
  }

  void visitInterfaceDecl(const nodes::InterfaceDeclNode *node) {
    printLine("InterfaceDecl " + getLocationString(node->getLocation()), BLUE);
    withIndent([&]() {
      printLine("Name: '" + node->getName() + "'");

      // Print if zerocast
      if (node->isZeroCast()) {
        printLine("ZeroCast: true");
      }

      // Check if this is a generic interface
      if (auto genericInterface =
              dynamic_cast<const nodes::GenericInterfaceDeclNode *>(node)) {
        if (!genericInterface->getGenericParams().empty()) {
          printLine("Generic Parameters:");
          withIndent([&]() {
            for (const auto &param : genericInterface->getGenericParams()) {
              visitType(param.get());
            }
          });
        }
      }

      // Print extended interfaces
      const auto &extended = node->getExtendedInterfaces();
      if (!extended.empty()) {
        printLine("Extends:");
        withIndent([&]() {
          for (const auto &iface : extended) {
            visitType(iface.get());
          }
        });
      }

      // Print members
      const auto &members = node->getMembers();
      if (!members.empty()) {
        printLine("Members:");
        withIndent([&]() {
          for (const auto &member : members) {
            print(member);
          }
        });
      }
    });
  }

  void visitParameter(const nodes::ParameterNode *node) {
    printLine("Parameter '" + node->getName() + "' " +
                  getLocationString(node->getLocation()),
              YELLOW);
    withIndent([&]() {
      if (node->getType()) {
        printLine("Type:");
        withIndent([&]() { visitType(node->getType().get()); });
      }
      if (node->isRef())
        printLine("Modifier: ref");
      if (node->isConst())
        printLine("Modifier: const");
      if (node->getDefaultValue()) {
        printLine("Default Value:");
        withIndent([&]() { visitExpr(node->getDefaultValue().get()); });
      }
    });
  }

  void visitTypedefDecl(const nodes::TypedefDeclNode *node) {
    printLine("TypedefDecl " + getLocationString(node->getLocation()), BLUE);
    withIndent([&]() {
      printLine("Name: '" + node->getName() + "'");

      if (node->getAliasedType()) {
        printLine("Aliased Type:");
        withIndent([&]() { visitType(node->getAliasedType().get()); });
      }

      // Print attributes if present
      const auto &attributes = node->getAttributes();
      if (!attributes.empty()) {
        printLine("Attributes:");
        withIndent([&]() {
          for (const auto &attr : attributes) {
            visitAttribute(attr.get());
          }
        });
      }
    });
  }

  //---------------------------------------------------------------------------
  // Visitor Functions for Statement and Expression Nodes
  //---------------------------------------------------------------------------

  void visitBlock(const nodes::BlockNode *node) {
    printLine("Block " + getLocationString(node->getLocation()));
    withIndent([&]() {
      for (const auto &stmt : node->getStatements())
        visitStmt(stmt.get());
    });
  }

  void visitStmt(const nodes::StatementNode *stmt) {
    if (!stmt) {
      printLine("null-statement", RED);
      return;
    }
    if (auto exprStmt = dynamic_cast<const nodes::ExpressionStmtNode *>(stmt))
      visitExprStmt(exprStmt);
    else if (auto returnStmt =
                 dynamic_cast<const nodes::ReturnStmtNode *>(stmt))
      visitReturnStmt(returnStmt);
    else if (auto ifStmt = dynamic_cast<const nodes::IfStmtNode *>(stmt))
      visitIfStmt(ifStmt);
    else if (auto declStmt =
                 dynamic_cast<const nodes::DeclarationStmtNode *>(stmt))
      visitDeclStmt(declStmt);
    else if (auto whileStmt = dynamic_cast<const nodes::WhileStmtNode *>(stmt))
      visitWhileStmt(whileStmt);
    else if (auto doWhileStmt =
                 dynamic_cast<const nodes::DoWhileStmtNode *>(stmt))
      visitDoWhileStmt(doWhileStmt);
    else if (auto forStmt = dynamic_cast<const nodes::ForStmtNode *>(stmt))
      visitForStmt(forStmt);
    else if (auto forOfStmt = dynamic_cast<const nodes::ForOfStmtNode *>(stmt))
      visitForOfStmt(forOfStmt);
    else if (auto blockStmt = dynamic_cast<const nodes::BlockNode *>(stmt))
      visitBlock(blockStmt);
    else if (auto breakStmt = dynamic_cast<const nodes::BreakStmtNode *>(stmt))
      visitBreakStmt(breakStmt);
    else if (auto continueStmt =
                 dynamic_cast<const nodes::ContinueStmtNode *>(stmt))
      visitContinueStmt(continueStmt);
    else if (auto tryStmt = dynamic_cast<const nodes::TryStmtNode *>(stmt))
      visitTryStmt(tryStmt);
    else if (auto throwStmt = dynamic_cast<const nodes::ThrowStmtNode *>(stmt))
      visitThrowStmt(throwStmt);
    else if (auto switchStmt =
                 dynamic_cast<const nodes::SwitchStmtNode *>(stmt)) {
      std::cout << "visiting switch statement\n";
      visitSwitchStmt(switchStmt);
    } else if (auto asmStmt =
                   dynamic_cast<const nodes::AssemblyStmtNode *>(stmt)) {
      visitAsmStmt(asmStmt);
    } else if (auto labeledStmt =
                   dynamic_cast<const nodes::LabeledStatementNode *>(stmt)) {
      visitLabeledStmt(labeledStmt);
    } else {
      indent();
      std::cout << RED << "Unknown statement type at "
                << stmt->getLocation().getLine() << ":"
                << stmt->getLocation().getColumn();
      std::cout << " (typeid: " << typeid(*stmt).name() << ")" << RESET << "\n";
    }
  }

  void visitExprStmt(const nodes::ExpressionStmtNode *node) {
    printLine("ExpressionStatement " + getLocationString(node->getLocation()));
    withIndent([&]() { visitExpr(node->getExpression().get()); });
  }

  void visitAttribute(const nodes::AttributeNode *node) {
    indent();
    std::cout << "Attribute: " << node->getName();
    if (node->getArgument()) {
      std::cout << " (";
      visitExpr(node->getArgument().get());
      std::cout << ")";
    }
    std::cout << "\n";
  }

  // Updated visitExpr to properly output assignment targets (especially
  // MemberAccess).
  void visitExpr(const nodes::ExpressionNode *expr) {
    if (!expr) {
      printLine("null-expression", RED);
      return;
    }
    // Literal expression.
    if (auto literal =
            dynamic_cast<const nodes::LiteralExpressionNode *>(expr)) {
      indent();
      std::cout << "Literal: '" << literal->getValue() << "' "
                << getLocationString(literal->getLocation()) << "\n";
    }
    // Binary expression.
    else if (auto binary =
                 dynamic_cast<const nodes::BinaryExpressionNode *>(expr)) {
      indent();
      std::cout << "BinaryExpression: "
                << tokenTypeToString(binary->getExpressionType()) << " "
                << getLocationString(binary->getLocation()) << "\n";
      withIndent([&]() {
        printLine("Left:");
        withIndent([&]() { visitExpr(binary->getLeft().get()); });
        printLine("Right:");
        withIndent([&]() { visitExpr(binary->getRight().get()); });
      });
    }
    // Identifier expression.
    else if (auto ident =
                 dynamic_cast<const nodes::IdentifierExpressionNode *>(expr)) {
      indent();
      std::cout << "Identifier: '" << ident->getName() << "' "
                << getLocationString(ident->getLocation()) << "\n";
    }
    // Member access expression.
    else if (auto member =
                 dynamic_cast<const nodes::MemberExpressionNode *>(expr)) {
      indent();
      std::cout << "MemberAccess: ";
      // If the object is 'this', print it directly.
      if (dynamic_cast<const nodes::ThisExpressionNode *>(
              member->getObject().get()))
        std::cout << "this";
      else
        visitExpr(member->getObject().get());
      std::cout << "." << member->getMember() << " "
                << getLocationString(member->getLocation()) << "\n";
    }
    // Assignment expression.
    else if (auto assign =
                 dynamic_cast<const nodes::AssignmentExpressionNode *>(expr)) {
      indent();
      std::cout << "Assignment: "
                << tokenTypeToString(assign->getExpressionType()) << " "
                << getLocationString(assign->getLocation()) << "\n";
      withIndent([&]() {
        printLine("Target:");
        withIndent([&]() { visitExpr(assign->getTarget().get()); });
        printLine("Value:");
        withIndent([&]() { visitExpr(assign->getValue().get()); });
      });
    }
    // Unary expression.
    else if (auto unary =
                 dynamic_cast<const nodes::UnaryExpressionNode *>(expr)) {
      visitUnaryExpr(unary);
    } else if (auto newExpr =
                   dynamic_cast<const nodes::NewExpressionNode *>(expr)) {
      visitNewExpr(newExpr);
    } else if (auto callExpt =
                   dynamic_cast<const nodes::CallExpressionNode *>(expr)) {
      visitCallExpt(callExpt);
    }
    // Generic expression fallback.
    else {
      indent();
      std::cout << "Expression: "
                << tokenTypeToString(expr->getExpressionType()) << " "
                << getLocationString(expr->getLocation()) << "\n";
    }
  }

  void visitType(const nodes::TypeNode *type) {
    if (!type) {
      printLine("null-type", RED);
      return;
    }
    indent();
    std::cout << type->toString() << "\n";
    withIndent([&]() {
      if (auto arrType = dynamic_cast<const nodes::ArrayTypeNode *>(type)) {
        printLine("ElementType:");
        withIndent([&]() { visitType(arrType->getElementType().get()); });
        if (arrType->getSize()) {
          printLine("Size:");
          withIndent([&]() { visitExpr(arrType->getSize().get()); });
        }
      } else if (auto ptrType =
                     dynamic_cast<const nodes::PointerTypeNode *>(type)) {
        printLine("BaseType:");
        withIndent([&]() { visitType(ptrType->getBaseType().get()); });
      }
    });
  }

  void visitWhileStmt(const nodes::WhileStmtNode *node) {
    printLine("While " + getLocationString(node->getLocation()));
    withIndent([&]() {
      printLine("Condition:");
      withIndent([&]() { visitExpr(node->getCondition().get()); });
      printLine("Body:");
      withIndent([&]() { visitStmt(node->getBody().get()); });
    });
  }

  void visitDoWhileStmt(const nodes::DoWhileStmtNode *node) {
    printLine("DoWhile " + getLocationString(node->getLocation()));
    withIndent([&]() {
      printLine("Body:");
      withIndent([&]() { visitStmt(node->getBody().get()); });
      printLine("Condition:");
      withIndent([&]() { visitExpr(node->getCondition().get()); });
    });
  }

  void visitBreakStmt(const nodes::BreakStmtNode *node) {
    indent();
    std::cout << "Break";
    if (!node->getLabel().empty())
      std::cout << " " << node->getLabel();
    std::cout << " " << getLocationString(node->getLocation()) << "\n";
  }

  void visitContinueStmt(const nodes::ContinueStmtNode *node) {
    indent();
    std::cout << "Continue";
    if (!node->getLabel().empty())
      std::cout << " " << node->getLabel();
    std::cout << " " << getLocationString(node->getLocation()) << "\n";
  }

  void visitReturnStmt(const nodes::ReturnStmtNode *node) {
    printLine("Return " + getLocationString(node->getLocation()));
    if (node->getValue()) {
      withIndent([&]() { visitExpr(node->getValue().get()); });
    }
  }

  void visitIfStmt(const nodes::IfStmtNode *node) {
    printLine("If " + getLocationString(node->getLocation()));
    withIndent([&]() {
      printLine("Condition:");
      withIndent([&]() { visitExpr(node->getCondition().get()); });
      printLine("Then:");
      withIndent([&]() { visitStmt(node->getThenBranch().get()); });
      if (node->getElseBranch()) {
        printLine("Else:");
        withIndent([&]() { visitStmt(node->getElseBranch().get()); });
      }
    });
  }

  void visitForStmt(const nodes::ForStmtNode *node) {
    printLine("For " + getLocationString(node->getLocation()));
    withIndent([&]() {
      printLine("Initializer:");
      withIndent([&]() {
        if (node->getInitializer())
          visitStmt(node->getInitializer().get());
        else
          printLine("<empty>");
      });
      printLine("Condition:");
      withIndent([&]() {
        if (node->getCondition())
          visitExpr(node->getCondition().get());
        else
          printLine("<empty>");
      });
      printLine("Increment:");
      withIndent([&]() {
        if (node->getIncrement())
          visitExpr(node->getIncrement().get());
        else
          printLine("<empty>");
      });
      printLine("Body:");
      withIndent([&]() { visitStmt(node->getBody().get()); });
    });
  }

  void visitArrayLiteral(const nodes::ArrayLiteralNode *node) {
    indent();
    std::cout << "ArrayLiteral " << getLocationString(node->getLocation())
              << "\n";
    indentLevel_++;
    indent();
    std::cout << "Elements:\n";
    indentLevel_++;
    for (const auto &element : node->getElements()) {
      visitExpr(element.get());
    }
    indentLevel_--;
    indentLevel_--;
  }

  void visitForOfStmt(const nodes::ForOfStmtNode *node) {
    indent();
    std::cout << "ForOf " << getLocationString(node->getLocation()) << "\n";
    indentLevel_++;
    indent();
    std::cout << (node->isConst() ? "const " : "let ") << node->getIdentifier()
              << "\n";
    indent();
    std::cout << "Iterable:\n";
    indentLevel_++;
    visitExpr(node->getIterable().get());
    indentLevel_--;
    indent();
    std::cout << "Body:\n";
    indentLevel_++;
    visitStmt(node->getBody().get());
    indentLevel_--;
    indentLevel_--;
  }

  void visitAsmStmt(const nodes::AssemblyStmtNode *node) {
    indent();
    printLine("Assembly Statement: " + node->getCode());
    withIndent([&]() {
      for (const auto &constraint : node->getConstraints()) {
        printLine(constraint);
      }
    });
  }

  void visitUnaryExpr(const nodes::UnaryExpressionNode *node) {
    indent();
    std::cout << "UnaryExpression "
              << (node->isPrefix() ? "(prefix) " : "(postfix) ")
              << tokenTypeToString(node->getExpressionType()) << " "
              << getLocationString(node->getLocation()) << "\n";
    withIndent([&]() {
      printLine("Operand:");
      withIndent([&]() { visitExpr(node->getOperand().get()); });
    });
  }
  // MethodDeclNode
  void visitDeclStmt(const nodes::DeclarationStmtNode *node) {
    printLine("Declaration Statement " +
              getLocationString(node->getLocation()));
    withIndent([&]() {
      if (auto varDecl = std::dynamic_pointer_cast<nodes::VarDeclNode>(
              node->getDeclaration()))
        visitVarDecl(varDecl.get());
      else if (auto funcDecl =
                   std::dynamic_pointer_cast<nodes::FunctionDeclNode>(
                       node->getDeclaration()))
        visitFuncDecl(funcDecl.get());
      else if (auto methodDecl =
                   std::dynamic_pointer_cast<nodes::MethodDeclNode>(
                       node->getDeclaration()))
        visitMethodDecl(methodDecl.get());
      else
        printLine("Unknown declaration type", RED);
    });
  }

  void visitTryStmt(const nodes::TryStmtNode *node) {
    printLine("Try " + getLocationString(node->getLocation()));
    withIndent([&]() {
      printLine("Try Block:");
      withIndent([&]() { visitStmt(node->getTryBlock().get()); });
      const auto &catchClauses = node->getCatchClauses();
      if (!catchClauses.empty()) {
        printLine("Catch Clauses:");
        withIndent([&]() {
          for (const auto &clause : catchClauses) {
            printLine("Catch Parameter: '" + clause.parameter + "'");
            if (clause.parameterType) {
              withIndent([&]() {
                printLine("Parameter Type:");
                withIndent([&]() { visitType(clause.parameterType.get()); });
              });
            }
            printLine("Catch Body:");
            withIndent([&]() { visitStmt(clause.body.get()); });
          }
        });
      }
      if (node->getFinallyBlock()) {
        printLine("Finally Block:");
        withIndent([&]() { visitStmt(node->getFinallyBlock().get()); });
      }
    });
  }

  void visitNewExpr(const nodes::NewExpressionNode *node) {
    indent();
    std::cout << "NewExpression: " << node->getClassName() << " "
              << getLocationString(node->getLocation()) << "\n";
    withIndent([&]() {
      const auto &args = node->getArguments();
      if (!args.empty()) {
        printLine("Arguments:");
        withIndent([&]() {
          for (const auto &arg : args) {
            visitExpr(arg.get());
          }
        });
      }
    });
  }

  void visitCallExpt(const nodes::CallExpressionNode *node) {
    indent();
    if (auto identNode = dynamic_cast<nodes::IdentifierExpressionNode *>(
            node->getCallee().get())) {
      // For simple function calls like: functionName()
      std::string functionName = identNode->getName();

      std::cout << "CallExpression: " << functionName << "\n";
    }
    withIndent([&]() {
      const auto &typeArgs = node->getTypeArguments();
      if (!typeArgs.empty()) {
        printLine("Generic Arguments: ");
        withIndent([&]() {
          indent();
          for (const auto &typeArg : typeArgs) {
            std::cout << typeArg << "\n";
          }
        });
      }
      const auto &args = node->getArguments();
      if (!args.empty()) {
        printLine("Arguments: ");
        withIndent([&]() {
          for (const auto &arg : args) {
            visitExpr(arg.get());
          }
        });
      }
    });
  }

  void visitSwitchStmt(const nodes::SwitchStmtNode *node) {
    printLine("Switch " + getLocationString(node->getLocation()));
    withIndent([&]() {
      printLine("Expression:");
      withIndent([&]() { visitExpr(node->getExpression().get()); });
      const auto &cases = node->getCases();
      if (!cases.empty()) {
        printLine("Cases:");
        withIndent([&]() {
          for (const auto &caseItem : cases) {
            if (caseItem.isDefault) {
              printLine("Default Case:");
            } else {
              printLine("Case:");
              withIndent([&]() {
                printLine("Value:");
                withIndent([&]() { visitExpr(caseItem.value.get()); });
              });
            }
            if (!caseItem.body.empty()) {
              printLine("Body:");
              withIndent([&]() {
                for (const auto &stmt : caseItem.body) {
                  visitStmt(stmt.get());
                }
              });
            }
          }
        });
      }
    });
  }

  void visitPropertyDecl(const nodes::PropertyDeclNode *node) {
    printLine("PropertyDecl " + getLocationString(node->getLocation()), GREEN);
    withIndent([&]() {
      printLine("Access: " + tokenTypeToString(node->getAccessModifier()));
      printLine("Name: '" + node->getName() + "'");
      printLine(std::string("Kind: ") +
                (node->getKind() == nodes::PropertyKind::Getter ? "Getter"
                                                                : "Setter"));
      if (node->getPropertyType()) {
        printLine("Property Type:");
        withIndent([&]() { visitType(node->getPropertyType().get()); });
      }

      if (node->getBody()) {
        printLine("Body:");
        withIndent([&]() { visitBlock(node->getBody().get()); });
      }
    });
  }

  void visitThrowStmt(const nodes::ThrowStmtNode *node) {
    printLine("Throw " + getLocationString(node->getLocation()));
    withIndent([&]() { visitExpr(node->getValue().get()); });
  }

  void visitLabeledStmt(const nodes::LabeledStatementNode *node) {
    printLine("Labeled Statement: " + node->getLabel() + " " +
              getLocationString(node->getLocation()));
    withIndent([&]() { visitStmt(node->getStatement().get()); });
  }

  void visitMethodSignature(const nodes::MethodSignatureNode *node) {
    printLine("MethodSignature '" + node->getName() + "' " +
                  getLocationString(node->getLocation()),
              YELLOW);
    withIndent([&]() {
      printLine("Access: " + tokenTypeToString(node->getAccessModifier()));

      const auto &parameters = node->getParameters();
      if (!parameters.empty()) {
        printLine("Parameters:");
        withIndent([&]() {
          for (const auto &param : parameters)
            visitParameter(param.get());
        });
      }

      if (node->getReturnType()) {
        printLine("Return Type:");
        withIndent([&]() { visitType(node->getReturnType().get()); });
      }

      if (!node->getThrowsTypes().empty()) {
        printLine("Throws:");
        withIndent([&]() {
          for (const auto &throwType : node->getThrowsTypes())
            visitType(throwType.get());
        });
      }
    });
  }

  void visitPropertySignature(const nodes::PropertySignatureNode *node) {
    printLine("PropertySignature '" + node->getName() + "' " +
                  getLocationString(node->getLocation()),
              YELLOW);
    withIndent([&]() {
      printLine("Access: " + tokenTypeToString(node->getAccessModifier()));

      if (node->getType()) {
        printLine("Type:");
        withIndent([&]() { visitType(node->getType().get()); });
      }

      if (node->hasGetter())
        printLine("Has Getter: true");
      if (node->hasSetter())
        printLine("Has Setter: true");
    });
  }

  void visitNamespaceDecl(const nodes::NamespaceDeclNode *node) {
    printLine("NamespaceDecl " + getLocationString(node->getLocation()), BLUE);
    withIndent([&]() {
      // Print namespace name
      printLine("Name: '" + node->getName() + "'");

      // Print all declarations inside the namespace
      const auto &declarations = node->getDeclarations();
      if (!declarations.empty()) {
        printLine("Declarations:");
        withIndent([&]() {
          for (const auto &decl : declarations) {
            print(decl); // Recursively print each declaration
          }
        });
      }
    });
  }

  //---------------------------------------------------------------------------
  // Utility Functions
  //---------------------------------------------------------------------------

  static std::string tokenTypeToString(tokens::TokenType type) {
    switch (type) {
    case tokens::TokenType::PLUS:
      return "+";
    case tokens::TokenType::MINUS:
      return "-";
    case tokens::TokenType::STAR:
      return "*";
    case tokens::TokenType::SLASH:
      return "/";
    case tokens::TokenType::EQUALS:
      return "=";
    case tokens::TokenType::GREATER:
      return ">";
    case tokens::TokenType::LESS:
      return "<";
    case tokens::TokenType::GREATER_EQUALS:
      return ">=";
    case tokens::TokenType::LESS_EQUALS:
      return "<=";
    case tokens::TokenType::PLUS_EQUALS:
      return "+=";
    case tokens::TokenType::MINUS_EQUALS:
      return "-=";
    case tokens::TokenType::STAR_EQUALS:
      return "*=";
    case tokens::TokenType::SLASH_EQUALS:
      return "/=";
    case tokens::TokenType::PLUS_PLUS:
      return "++";
    case tokens::TokenType::MINUS_MINUS:
      return "--";
    case tokens::TokenType::PERCENT:
      return "%";
    case tokens::TokenType::OF:
      return "of";
    case tokens::TokenType::STACK:
      return "#stack";
    case tokens::TokenType::HEAP:
      return "#heap";
    case tokens::TokenType::STATIC:
      return "#static";
    case tokens::TokenType::INT:
      return "int";
    case tokens::TokenType::FLOAT:
      return "float";
    case tokens::TokenType::BOOLEAN:
      return "bool";
    case tokens::TokenType::STRING:
      return "string";
    case tokens::TokenType::VOID:
      return "void";
    case tokens::TokenType::PUBLIC:
      return "public";
    case tokens::TokenType::PRIVATE:
      return "private";
    case tokens::TokenType::PROTECTED:
      return "protected";
    case tokens::TokenType::INLINE:
      return "#inline";
    case tokens::TokenType::VIRTUAL:
      return "#virtual";
    case tokens::TokenType::UNSAFE:
      return "#unsafe";
    case tokens::TokenType::SIMD:
      return "#simd";
    case tokens::TokenType::PACKED:
      return "#packed";
    case tokens::TokenType::ABSTRACT:
      return "#abstract";
    case tokens::TokenType::ALIGNED:
      return "#aligned";
    default:
      return std::to_string(static_cast<int>(type));
    }
  }

  std::string modifierToString(tokens::TokenType modifier) {
    switch (modifier) {
    case tokens::TokenType::INLINE:
      return "#inline";
    case tokens::TokenType::VIRTUAL:
      return "#virtual";
    case tokens::TokenType::UNSAFE:
      return "#unsafe";
    case tokens::TokenType::SIMD:
      return "#simd";
    default:
      return "unknown";
    }
  }

public:
  void print(const parser::AST &ast) {
    std::cout << "\nAbstract Syntax Tree:\n" << std::string(80, '-') << "\n";
    const auto &nodes = ast.getNodes();
    if (nodes.empty()) {
      printLine("Empty AST", RED);
      return;
    }
    for (const auto &node : nodes) {
      if (auto classDecl =
              std::dynamic_pointer_cast<nodes::ClassDeclNode>(node))
        visitClassDecl(classDecl.get());
      else if (auto methodDecl =
                   std::dynamic_pointer_cast<nodes::MethodDeclNode>(node))
        visitMethodDecl(methodDecl.get());
      else if (auto ctorDecl =
                   std::dynamic_pointer_cast<nodes::ConstructorDeclNode>(node))
        visitConstructorDecl(ctorDecl.get());
      else if (auto fieldDecl =
                   std::dynamic_pointer_cast<nodes::FieldDeclNode>(node))
        visitFieldDecl(fieldDecl.get());
      else if (auto funcDecl =
                   std::dynamic_pointer_cast<nodes::FunctionDeclNode>(node))
        visitFuncDecl(funcDecl.get());
      else if (auto varDecl =
                   std::dynamic_pointer_cast<nodes::VarDeclNode>(node))
        visitVarDecl(varDecl.get());
      else if (auto stmt =
                   std::dynamic_pointer_cast<nodes::StatementNode>(node))
        visitStmt(stmt.get());
      else if (auto exprStmt =
                   std::dynamic_pointer_cast<nodes::ExpressionStmtNode>(node))
        visitExprStmt(exprStmt.get());
      else if (auto expr =
                   std::dynamic_pointer_cast<nodes::ExpressionNode>(node))
        visitExpr(expr.get());
      else if (auto throwStmt =
                   std::dynamic_pointer_cast<nodes::ThrowStmtNode>(node))
        visitThrowStmt(throwStmt.get());
      else if (auto propertyDecl =
                   std::dynamic_pointer_cast<nodes::PropertyDeclNode>(node))
        visitPropertyDecl(propertyDecl.get());
      else if (auto enumDecl =
                   std::dynamic_pointer_cast<nodes::EnumDeclNode>(node))
        visitEnumDecl(enumDecl.get());
      else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<nodes::InterfaceDeclNode>(node))
        visitInterfaceDecl(interfaceDecl.get());
      else if (auto typedefDecl =
                   std::dynamic_pointer_cast<nodes::TypedefDeclNode>(node))
        visitTypedefDecl(typedefDecl.get());
      else if (auto namespaceDecl =
                   std::dynamic_pointer_cast<nodes::NamespaceDeclNode>(node))
        visitNamespaceDecl(namespaceDecl.get());
      else {
        indent();
        std::cout << RED << "Unknown node type at gfgfgf"
                  << node->getLocation().getLine() << ":"
                  << node->getLocation().getColumn() << RESET << "\n";
      }
    }
    std::cout << std::string(80, '-') << "\n";
  }

  void print(const nodes::NodePtr &node) {
    if (!node) {
      printLine("nullptr", RED);
      return;
    }
    if (auto classDecl = std::dynamic_pointer_cast<nodes::ClassDeclNode>(node))
      visitClassDecl(classDecl.get());
    else if (auto methodDecl =
                 std::dynamic_pointer_cast<nodes::MethodDeclNode>(node))
      visitMethodDecl(methodDecl.get());
    else if (auto ctorDecl =
                 std::dynamic_pointer_cast<nodes::ConstructorDeclNode>(node))
      visitConstructorDecl(ctorDecl.get());
    else if (auto fieldDecl =
                 std::dynamic_pointer_cast<nodes::FieldDeclNode>(node))
      visitFieldDecl(fieldDecl.get());
    else if (auto genericFunc =
                 std::dynamic_pointer_cast<nodes::GenericFunctionDeclNode>(
                     node))
      visitFuncDecl(genericFunc.get());
    else if (auto funcDecl =
                 std::dynamic_pointer_cast<nodes::FunctionDeclNode>(node))
      visitFuncDecl(funcDecl.get());
    else if (auto varDecl = std::dynamic_pointer_cast<nodes::VarDeclNode>(node))
      visitVarDecl(varDecl.get());
    else if (auto blockStmt = std::dynamic_pointer_cast<nodes::BlockNode>(node))
      visitBlock(blockStmt.get());
    else if (auto ifStmt = std::dynamic_pointer_cast<nodes::IfStmtNode>(node))
      visitIfStmt(ifStmt.get());
    else if (auto whileStmt =
                 std::dynamic_pointer_cast<nodes::WhileStmtNode>(node))
      visitWhileStmt(whileStmt.get());
    else if (auto doWhileStmt =
                 std::dynamic_pointer_cast<nodes::DoWhileStmtNode>(node))
      visitDoWhileStmt(doWhileStmt.get());
    else if (auto forStmt = std::dynamic_pointer_cast<nodes::ForStmtNode>(node))
      visitForStmt(forStmt.get());
    else if (auto forOfStmt =
                 std::dynamic_pointer_cast<nodes::ForOfStmtNode>(node))
      visitForOfStmt(forOfStmt.get());
    else if (auto breakStmt =
                 std::dynamic_pointer_cast<nodes::BreakStmtNode>(node))
      visitBreakStmt(breakStmt.get());
    else if (auto continueStmt =
                 std::dynamic_pointer_cast<nodes::ContinueStmtNode>(node))
      visitContinueStmt(continueStmt.get());
    else if (auto returnStmt =
                 std::dynamic_pointer_cast<nodes::ReturnStmtNode>(node))
      visitReturnStmt(returnStmt.get());
    else if (auto exprStmt =
                 std::dynamic_pointer_cast<nodes::ExpressionStmtNode>(node))
      visitExprStmt(exprStmt.get());
    else if (auto expr = std::dynamic_pointer_cast<nodes::ExpressionNode>(node))
      visitExpr(expr.get());
    else if (auto tryStmt = std::dynamic_pointer_cast<nodes::TryStmtNode>(node))
      visitTryStmt(tryStmt.get());
    else if (auto propertyDecl =
                 std::dynamic_pointer_cast<nodes::PropertyDeclNode>(node))
      visitPropertyDecl(propertyDecl.get());
    else if (auto propertyDecl =
                 std::dynamic_pointer_cast<nodes::PropertyDeclNode>(node))
      visitPropertyDecl(propertyDecl.get());
    // Add to your print method for NodePtr
    else if (auto methodSig =
                 std::dynamic_pointer_cast<nodes::MethodSignatureNode>(node))
      visitMethodSignature(methodSig.get());
    else if (auto propSig =
                 std::dynamic_pointer_cast<nodes::PropertySignatureNode>(node))
      visitPropertySignature(propSig.get());

    // Add to your print method for AST nodes too
    else if (auto methodSig =
                 std::dynamic_pointer_cast<nodes::MethodSignatureNode>(node))
      visitMethodSignature(methodSig.get());
    else if (auto propSig =
                 std::dynamic_pointer_cast<nodes::PropertySignatureNode>(node))
      visitPropertySignature(propSig.get());
    else if (auto typedefDecl =
                 std::dynamic_pointer_cast<nodes::TypedefDeclNode>(node))
      visitTypedefDecl(typedefDecl.get());
    else if (auto namespaceDecl =
                 std::dynamic_pointer_cast<nodes::NamespaceDeclNode>(node))
      visitNamespaceDecl(namespaceDecl.get());
    else {
      indent();
      std::cout << RED << "Unknown node type at "
                << node->getLocation().getLine() << ":"
                << node->getLocation().getColumn() << RESET << "\n";
    }
  }
};

} // namespace core
