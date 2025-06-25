/*****************************************************************************
 * File: statement_nodes.h
 * Description: AST node definitions for statements in TSPP language
 *****************************************************************************/

#pragma once
#include "base_node.h"
#include "expression_nodes.h"
#include "parser/nodes/declaration_nodes.h"
#include "parser/nodes/type_nodes.h"
#include <memory>
#include <vector>

namespace nodes {

/**
 * Base class for all statement nodes
 */
class StatementNode : public BaseNode {
public:
  explicit StatementNode(const core::SourceLocation &loc) : BaseNode(loc) {}
  virtual ~StatementNode() = default;
};

using StmtPtr = std::shared_ptr<StatementNode>;

/**
 * Represents a single case in a switch statement
 */
struct SwitchCase {
  bool isDefault;            // Whether this is a default case
  ExpressionPtr value;       // The case expression (nullptr for default)
  std::vector<StmtPtr> body; // The statements in this case

  SwitchCase(bool isDefault, ExpressionPtr value, std::vector<StmtPtr> body)
      : isDefault(isDefault), value(std::move(value)), body(std::move(body)) {}
};

class DeclarationStmtNode : public StatementNode {
public:
  DeclarationStmtNode(DeclPtr declaration, const core::SourceLocation &loc)
      : StatementNode(loc), declaration_(std::move(declaration)) {}

  const DeclPtr &getDeclaration() const { return declaration_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  DeclPtr declaration_;
};

/**
 * Block statement: { statements... }
 */
class BlockNode : public StatementNode {
public:
  BlockNode(std::vector<StmtPtr> statements, const core::SourceLocation &loc)
      : StatementNode(loc), statements_(std::move(statements)) {}

  const std::vector<StmtPtr> &getStatements() const { return statements_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::vector<StmtPtr> statements_;
};

/**
 * Expression statement: expr;
 */
class ExpressionStmtNode : public StatementNode {
public:
  ExpressionStmtNode(ExpressionPtr expression, const core::SourceLocation &loc)
      : StatementNode(loc), expression_(std::move(expression)) {}

  ExpressionPtr getExpression() const { return expression_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  ExpressionPtr expression_;
};

/**
 * If statement: if (cond) stmt else stmt
 */
class IfStmtNode : public StatementNode {
public:
  IfStmtNode(ExpressionPtr condition, StmtPtr thenBranch, StmtPtr elseBranch,
             const core::SourceLocation &loc)
      : StatementNode(loc), condition_(std::move(condition)),
        thenBranch_(std::move(thenBranch)), elseBranch_(std::move(elseBranch)) {
  }

  ExpressionPtr getCondition() const { return condition_; }
  StmtPtr getThenBranch() const { return thenBranch_; }
  StmtPtr getElseBranch() const { return elseBranch_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  ExpressionPtr condition_;
  StmtPtr thenBranch_;
  StmtPtr elseBranch_;
};

/**
 * While statement: while (cond) stmt
 */
class WhileStmtNode : public StatementNode {
public:
  WhileStmtNode(ExpressionPtr condition, StmtPtr body,
                const core::SourceLocation &loc)
      : StatementNode(loc), condition_(std::move(condition)),
        body_(std::move(body)) {}

  ExpressionPtr getCondition() const { return condition_; }
  StmtPtr getBody() const { return body_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  ExpressionPtr condition_;
  StmtPtr body_;
};

/**
 * Do-While statement: do stmt while (cond);
 */
class DoWhileStmtNode : public StatementNode {
public:
  DoWhileStmtNode(StmtPtr body, ExpressionPtr condition,
                  const core::SourceLocation &loc)
      : StatementNode(loc), body_(std::move(body)),
        condition_(std::move(condition)) {}

  StmtPtr getBody() const { return body_; }
  ExpressionPtr getCondition() const { return condition_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  StmtPtr body_;
  ExpressionPtr condition_;
};

/**
 * For statement: for (init; cond; incr) stmt
 */
class ForStmtNode : public StatementNode {
public:
  ForStmtNode(StmtPtr init, ExpressionPtr condition, ExpressionPtr increment,
              StmtPtr body, const core::SourceLocation &loc)
      : StatementNode(loc), initializer_(std::move(init)),
        condition_(std::move(condition)), increment_(std::move(increment)),
        body_(std::move(body)) {}

  StmtPtr getInitializer() const { return initializer_; }
  ExpressionPtr getCondition() const { return condition_; }
  ExpressionPtr getIncrement() const { return increment_; }
  StmtPtr getBody() const { return body_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  StmtPtr initializer_;
  ExpressionPtr condition_;
  ExpressionPtr increment_;
  StmtPtr body_;
};

/**
 * For-of statement: for (let x of expr) stmt
 */
class ForOfStmtNode : public StatementNode {
public:
  ForOfStmtNode(bool isConst, const std::string &identifier,
                ExpressionPtr iterable, StmtPtr body,
                const core::SourceLocation &loc)
      : StatementNode(loc), isConst_(isConst), identifier_(identifier),
        iterable_(std::move(iterable)), body_(std::move(body)) {}

  bool isConst() const { return isConst_; }
  const std::string &getIdentifier() const { return identifier_; }
  ExpressionPtr getIterable() const { return iterable_; }
  StmtPtr getBody() const { return body_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  bool isConst_;
  std::string identifier_;
  ExpressionPtr iterable_;
  StmtPtr body_;
};

/**
 * Break statement: break label?;
 */
class BreakStmtNode : public StatementNode {
public:
  BreakStmtNode(std::string label, const core::SourceLocation &loc)
      : StatementNode(loc), label_(std::move(label)) {}

  const std::string &getLabel() const { return label_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::string label_; // Optional label
};

/**
 * Continue statement: continue label?;
 */
class ContinueStmtNode : public StatementNode {
public:
  ContinueStmtNode(std::string label, const core::SourceLocation &loc)
      : StatementNode(loc), label_(std::move(label)) {}

  const std::string &getLabel() const { return label_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::string label_; // Optional label
};

/**
 * Return statement: return expr?;
 */
class ReturnStmtNode : public StatementNode {
public:
  ReturnStmtNode(ExpressionPtr value, const core::SourceLocation &loc)
      : StatementNode(loc), value_(std::move(value)) {}

  ExpressionPtr getValue() const { return value_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  ExpressionPtr value_; // Optional return value
};

/**
 * Try-catch-finally statement
 */
class TryStmtNode : public StatementNode {
public:
  struct CatchClause {
    std::string parameter;
    TypePtr parameterType;
    StmtPtr body;
  };

  TryStmtNode(StmtPtr tryBlock, std::vector<CatchClause> catchClauses,
              StmtPtr finallyBlock, const core::SourceLocation &loc)
      : StatementNode(loc), tryBlock_(std::move(tryBlock)),
        catchClauses_(std::move(catchClauses)),
        finallyBlock_(std::move(finallyBlock)) {}

  StmtPtr getTryBlock() const { return tryBlock_; }
  const std::vector<CatchClause> &getCatchClauses() const {
    return catchClauses_;
  }
  StmtPtr getFinallyBlock() const { return finallyBlock_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  StmtPtr tryBlock_;
  std::vector<CatchClause> catchClauses_;
  StmtPtr finallyBlock_; // Optional finally block
};

/**
 * Throw statement: throw expr;
 */
class ThrowStmtNode : public StatementNode {
public:
  ThrowStmtNode(ExpressionPtr value, const core::SourceLocation &loc)
      : StatementNode(loc), value_(std::move(value)) {}

  ExpressionPtr getValue() const { return value_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  ExpressionPtr value_;
};

/**
 * Switch statement: switch (expr) { case expr: ... default: ... }
 */
class SwitchStmtNode : public StatementNode {
public:
  SwitchStmtNode(ExpressionPtr expression, std::vector<SwitchCase> cases,
                 const core::SourceLocation &loc)
      : StatementNode(loc), expression_(std::move(expression)),
        cases_(std::move(cases)) {}

  ExpressionPtr getExpression() const { return expression_; }
  const std::vector<SwitchCase> &getCases() const { return cases_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  ExpressionPtr expression_;
  std::vector<SwitchCase> cases_;
};

/**
 * Assembly statement: #asm("...")
 */
class AssemblyStmtNode : public StatementNode {
public:
  AssemblyStmtNode(std::string code, std::vector<std::string> constraints,
                   const core::SourceLocation &loc)
      : StatementNode(loc), code_(std::move(code)),
        constraints_(std::move(constraints)) {}

  const std::string &getCode() const { return code_; }
  const std::vector<std::string> &getConstraints() const {
    return constraints_;
  }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::string code_;
  std::vector<std::string> constraints_;
};

/**
 * Labeled Statement node: loopStart: [statements]
 */
class LabeledStatementNode : public StatementNode {
public:
    LabeledStatementNode(const std::string& label, StmtPtr statement, 
                         const core::SourceLocation& loc)
        : StatementNode(loc), label_(label), statement_(std::move(statement)) {}

    const std::string& getLabel() const { return label_; }
    StmtPtr getStatement() const { return statement_; }
    
    bool accept(interface::BaseInterface* visitor) override {
        return visitor->visitParse();
    }

private:
    std::string label_;
    StmtPtr statement_;
};

// Forward declarations for statement visitors
class StmtVisitor {
public:
  virtual ~StmtVisitor() = default;
  virtual bool visitBlock(BlockNode *node) = 0;
  virtual bool visitExpressionStmt(ExpressionStmtNode *node) = 0;
  virtual bool visitIf(IfStmtNode *node) = 0;
  virtual bool visitSwitch(SwitchStmtNode *node) = 0;
  virtual bool visitWhile(WhileStmtNode *node) = 0;
  virtual bool visitDoWhile(DoWhileStmtNode *node) = 0;
  virtual bool visitFor(ForStmtNode *node) = 0;
  virtual bool visitForOf(ForOfStmtNode *node) = 0;
  virtual bool visitBreak(BreakStmtNode *node) = 0;
  virtual bool visitContinue(ContinueStmtNode *node) = 0;
  virtual bool visitReturn(ReturnStmtNode *node) = 0;
  virtual bool visitTry(TryStmtNode *node) = 0;
  virtual bool visitThrow(ThrowStmtNode *node) = 0;
  virtual bool visitAssembly(AssemblyStmtNode *node) = 0;
};

} // namespace nodes