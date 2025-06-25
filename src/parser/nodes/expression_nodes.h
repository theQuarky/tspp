/*****************************************************************************
 * File: expression_nodes.h
 * Description: AST expression node definitions based on TSPP grammar
 *****************************************************************************/

#pragma once
#include "base_node.h"
#include "tokens/token_type.h"
#include <vector>

namespace nodes {

class ParameterNode;
using ParamPtr = std::shared_ptr<ParameterNode>;
class BlockNode;
using BlockPtr = std::shared_ptr<BlockNode>;
class TypeNode;
using TypePtr = std::shared_ptr<TypeNode>;

/**
 * @brief Base class for all expression nodes
 */
class ExpressionNode : public BaseNode {
public:
  ExpressionNode(const core::SourceLocation &loc, tokens::TokenType type)
      : BaseNode(loc), expressionType_(type) {}

  tokens::TokenType getExpressionType() const { return expressionType_; }
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

protected:
  tokens::TokenType expressionType_;
};

using ExpressionPtr = std::shared_ptr<ExpressionNode>;

// Binary expressions (a + b, x * y)
class BinaryExpressionNode : public ExpressionNode {
public:
  BinaryExpressionNode(const core::SourceLocation &loc, tokens::TokenType op,
                       ExpressionPtr left, ExpressionPtr right)
      : ExpressionNode(loc, op), left_(std::move(left)),
        right_(std::move(right)) {}

  ExpressionPtr getLeft() const { return left_; }
  ExpressionPtr getRight() const { return right_; }

private:
  ExpressionPtr left_;
  ExpressionPtr right_;
};

// Unary expression (e.g., !x, -y, ++z)
class UnaryExpressionNode : public ExpressionNode {
public:
  UnaryExpressionNode(const core::SourceLocation &loc, tokens::TokenType op,
                      ExpressionPtr operand, bool isPrefix)
      : ExpressionNode(loc, op), operand_(std::move(operand)),
        isPrefix_(isPrefix) {}

  ExpressionPtr getOperand() const { return operand_; }
  bool isPrefix() const { return isPrefix_; }

private:
  ExpressionPtr operand_;
  bool isPrefix_;
};

// Literal expression (numbers, strings, booleans)
class LiteralExpressionNode : public ExpressionNode {
public:
  LiteralExpressionNode(const core::SourceLocation &loc, tokens::TokenType type,
                        std::string value)
      : ExpressionNode(loc, type), value_(std::move(value)) {}

  const std::string &getValue() const { return value_; }

private:
  std::string value_;
};

// Identifier expression (variable names, function names)
class IdentifierExpressionNode : public ExpressionNode {
public:
  IdentifierExpressionNode(const core::SourceLocation &loc, std::string name)
      : ExpressionNode(loc, tokens::TokenType::IDENTIFIER),
        name_(std::move(name)) {}

  const std::string &getName() const { return name_; }

private:
  std::string name_;
};

// Array literal expression [1, 2, 3]
class ArrayLiteralNode : public ExpressionNode {
public:
  ArrayLiteralNode(const core::SourceLocation &loc,
                   std::vector<ExpressionPtr> elements)
      : ExpressionNode(loc, tokens::TokenType::LEFT_BRACKET),
        elements_(std::move(elements)) {}

  const std::vector<ExpressionPtr> &getElements() const { return elements_; }

private:
  std::vector<ExpressionPtr> elements_;
};

// Conditional expression (ternary operator) a ? b : c
class ConditionalExpressionNode : public ExpressionNode {
public:
  ConditionalExpressionNode(const core::SourceLocation &loc,
                            ExpressionPtr condition, ExpressionPtr trueExpr,
                            ExpressionPtr falseExpr)
      : ExpressionNode(loc, tokens::TokenType::QUESTION),
        condition_(std::move(condition)), trueExpr_(std::move(trueExpr)),
        falseExpr_(std::move(falseExpr)) {}

  ExpressionPtr getCondition() const { return condition_; }
  ExpressionPtr getTrueExpression() const { return trueExpr_; }
  ExpressionPtr getFalseExpression() const { return falseExpr_; }

private:
  ExpressionPtr condition_;
  ExpressionPtr trueExpr_;
  ExpressionPtr falseExpr_;
};

// Assignment expression (a = b, x += y)
class AssignmentExpressionNode : public ExpressionNode {
public:
  AssignmentExpressionNode(const core::SourceLocation &loc,
                           tokens::TokenType op, ExpressionPtr target,
                           ExpressionPtr value)
      : ExpressionNode(loc, op), target_(std::move(target)),
        value_(std::move(value)) {}

  ExpressionPtr getTarget() const { return target_; }
  ExpressionPtr getValue() const { return value_; }

private:
  ExpressionPtr target_;
  ExpressionPtr value_;
};

// Call expression (function calls, method calls)
class CallExpressionNode : public ExpressionNode {
public:
  CallExpressionNode(const core::SourceLocation &loc, ExpressionPtr callee,
                     std::vector<ExpressionPtr> arguments,
                     std::vector<std::string> typeArguments = {})
      : ExpressionNode(loc, tokens::TokenType::LEFT_PAREN),
        callee_(std::move(callee)), arguments_(std::move(arguments)),
        typeArguments_(std::move(typeArguments)) {}

  ExpressionPtr getCallee() const { return callee_; }
  const std::vector<ExpressionPtr> &getArguments() const { return arguments_; }
  const std::vector<std::string> &getTypeArguments() const {
    return typeArguments_;
  }

private:
  ExpressionPtr callee_;
  std::vector<ExpressionPtr> arguments_;
  std::vector<std::string> typeArguments_; // New member for generic type args
};

// Member access expression (obj.member, ptr@member)
class MemberExpressionNode : public ExpressionNode {
public:
  MemberExpressionNode(const core::SourceLocation &loc, ExpressionPtr object,
                       std::string member, bool isPointer)
      : ExpressionNode(loc, isPointer ? tokens::TokenType::AT
                                      : tokens::TokenType::DOT),
        object_(std::move(object)), member_(std::move(member)),
        isPointer_(isPointer) {}

  ExpressionPtr getObject() const { return object_; }
  const std::string &getMember() const { return member_; }
  bool isPointer() const { return isPointer_; }

private:
  ExpressionPtr object_;
  std::string member_;
  bool isPointer_;
};

// Index expression (array[index])
class IndexExpressionNode : public ExpressionNode {
public:
  IndexExpressionNode(const core::SourceLocation &loc, ExpressionPtr array,
                      ExpressionPtr index)
      : ExpressionNode(loc, tokens::TokenType::LEFT_BRACKET),
        array_(std::move(array)), index_(std::move(index)) {}

  ExpressionPtr getArray() const { return array_; }
  ExpressionPtr getIndex() const { return index_; }

private:
  ExpressionPtr array_;
  ExpressionPtr index_;
};

// This expression
class ThisExpressionNode : public ExpressionNode {
public:
  explicit ThisExpressionNode(const core::SourceLocation &loc)
      : ExpressionNode(loc, tokens::TokenType::THIS) {}
  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }
};

// New expression (new Type(args))
class NewExpressionNode : public ExpressionNode {
public:
  NewExpressionNode(const core::SourceLocation &loc, std::string className,
                    std::vector<ExpressionPtr> arguments)
      : ExpressionNode(loc, tokens::TokenType::NEW),
        className_(std::move(className)), arguments_(std::move(arguments)) {}

  const std::string &getClassName() const { return className_; }
  const std::vector<ExpressionPtr> &getArguments() const { return arguments_; }

private:
  std::string className_;
  std::vector<ExpressionPtr> arguments_;
};

// Cast expression (cast<Type>expr)
class CastExpressionNode : public ExpressionNode {
public:
  CastExpressionNode(const core::SourceLocation &loc, std::string targetType,
                     ExpressionPtr expression)
      : ExpressionNode(loc, tokens::TokenType::IDENTIFIER),
        targetType_(std::move(targetType)), expression_(std::move(expression)) {
  }

  const std::string &getTargetType() const { return targetType_; }
  ExpressionPtr getExpression() const { return expression_; }

private:
  std::string targetType_;
  ExpressionPtr expression_;
};

// Compile-time expressions (#const, #sizeof, #alignof, #typeof)
class CompileTimeExpressionNode : public ExpressionNode {
public:
  CompileTimeExpressionNode(const core::SourceLocation &loc,
                            tokens::TokenType kind, ExpressionPtr operand)
      : ExpressionNode(loc, kind), operand_(std::move(operand)) {}

  ExpressionPtr getOperand() const { return operand_; }

private:
  ExpressionPtr operand_;
};

// Template specialization expression (Type<T>)
class TemplateSpecializationNode : public ExpressionNode {
public:
  TemplateSpecializationNode(const core::SourceLocation &loc,
                             ExpressionPtr base,
                             std::vector<std::string> typeArguments)
      : ExpressionNode(loc, tokens::TokenType::TEMPLATE),
        base_(std::move(base)), typeArguments_(std::move(typeArguments)) {}

  ExpressionPtr getBase() const { return base_; }
  const std::vector<std::string> &getTypeArguments() const {
    return typeArguments_;
  }

private:
  ExpressionPtr base_;
  std::vector<std::string> typeArguments_;
};

// Pointer expression (handling unsafe and aligned pointers)
class PointerExpressionNode : public ExpressionNode {
public:
  enum class PointerKind {
    Raw,    // @
    Unsafe, // @unsafe
    Aligned // @aligned(N)
  };

  PointerExpressionNode(const core::SourceLocation &loc, ExpressionPtr operand,
                        PointerKind kind, uint32_t alignment = 0)
      : ExpressionNode(loc, tokens::TokenType::AT),
        operand_(std::move(operand)), kind_(kind), alignment_(alignment) {}

  ExpressionPtr getOperand() const { return operand_; }
  PointerKind getKind() const { return kind_; }
  uint32_t getAlignment() const { return alignment_; }

private:
  ExpressionPtr operand_;
  PointerKind kind_;
  uint32_t alignment_; // Only used when kind is Aligned
};

/**
 * Attribute node for declaration modifiers (#inline, #stack, etc)
 */
class AttributeNode : public BaseNode {
public:
  AttributeNode(const std::string &name, ExpressionPtr argument,
                const core::SourceLocation &loc)
      : BaseNode(loc), name_(name), argument_(std::move(argument)) {}

  const std::string &getName() const { return name_; }
  ExpressionPtr getArgument() const { return argument_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::string name_; // Attribute name without '#' prefix
  ExpressionPtr
      argument_; // Optional argument for the attribute (can be nullptr)
};

/**
 * Function expression node (for anonymous functions)
 */
class FunctionExpressionNode : public ExpressionNode {
public:
  FunctionExpressionNode(std::vector<ParamPtr> parameters, TypePtr returnType,
                         BlockPtr body, const core::SourceLocation &loc)
      : ExpressionNode(loc, tokens::TokenType::FUNCTION),
        parameters_(std::move(parameters)), returnType_(std::move(returnType)),
        body_(std::move(body)) {}

  const std::vector<ParamPtr> &getParameters() const { return parameters_; }
  TypePtr getReturnType() const { return returnType_; }
  BlockPtr getBody() const { return body_; }

  bool accept(interface::BaseInterface *visitor) override {
    return visitor->visitParse();
  }

private:
  std::vector<ParamPtr> parameters_;
  TypePtr returnType_;
  BlockPtr body_;
};

} // namespace nodes