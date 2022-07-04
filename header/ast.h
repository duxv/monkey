#ifndef MONKEY_AST_H_
#define MONKEY_AST_H_
#include <string>
#include <vector>
#include "token.h"

namespace monkey {
/*
 * Interfaces
 * use pure class to imitate interface.
 */

enum NodeType {
    NodeProgram,
    ExprIdent,
    ExprInt,
    ExprBoolLit,
    ExprStringLit,
    ExprFuncLit,
    ExprArrayLit,
    ExprCall,
    ExprIndex,
    ExprPrefix,
    ExprInfix,
    ExprIf,
    ExprWhile,
    LetStmt,
    RefStmt,
    ReturnStmt,
    ExprStmt,
    BlockStmt,
};

class Node {
 public:
  virtual ~Node() { };
  virtual std::string TokenLiteral() = 0;
  virtual std::string String() = 0;
  virtual NodeType Type() = 0;
};

// here I haven't found any good solution to nesting interface.
class Statement : public Node {
 public:
  virtual void statementNode() = 0;
  virtual std::string TokenLiteral() = 0;
  virtual std::string String() = 0;
  virtual NodeType Type() = 0;
};

class Expression : public Node {
 public:
  virtual std::string TokenLiteral() = 0;
  virtual std::string String() = 0;
  virtual NodeType Type() = 0;
};

/*
 * Whole Program
 */
class Program : public Node {
 public:
  ~Program() {
    for(auto stmt : statements) {
      delete stmt;
    }
  }
  std::string TokenLiteral();
  std::string String();
  NodeType Type() { return NodeProgram; }

  std::vector<Statement*> statements;  
};

/*
 * Expressions
 */
class BlockStatement;

class Identifier : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String() { return value; }
  NodeType Type() { return ExprIdent; }
  
  Token token;
  std::string value;
};

class IntegerLiteral : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String() { return std::to_string(value); }
  NodeType Type() { return ExprInt; }
  
  Token token;
  int value;
};

class BooleanLiteral : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String() { return value ? "true" : "false"; }
  NodeType Type() { return ExprBoolLit; }
  
  Token token;
  bool value;
};

class StringLiteral : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String() { return value; }
  NodeType Type() { return ExprStringLit; }

  Token token;
  std::string value;
};

class FunctionLiteral : public Expression {
 public:
  ~FunctionLiteral() {
    for(auto param : parameters) {
      delete param;
    }
  }

  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprFuncLit; }
  
  Token token;
  std::vector<Identifier*> parameters;
  BlockStatement* body;
};

class ArrayLiteral : public Expression {
 public:
  ~ArrayLiteral() {
    for(auto elem : elements) {
      delete elem;
    }
  }

  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprArrayLit; }
  
  Token token;
  std::vector<Expression*> elements;
};

class CallExpression : public Expression {
 public:
  ~CallExpression() {
   for(auto arg : arguments) {
      delete arg;
    }
    delete function;
  }

  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprCall; }
  
  Token token;
  Expression* function;
  std::vector<Expression*> arguments;
};

class IndexExpression : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprIndex; }

  Token token;
  Expression* array;
  Expression* index;
};

class PrefixExpression : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprPrefix; }
  
  Token token;
  std::string op;
  Expression* right;
};

class InfixExpression : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprInfix; }
  
  Token token;
  Expression* left;
  std::string op;
  Expression* right;
};

class IfExpression : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprIf; }
  
  Token token;  // if
  Expression* condition;
  BlockStatement* consequence;
  BlockStatement* alternative;
};

class WhileExpression : public Expression {
 public:
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprWhile; }
  
  Token token;  // if
  Expression* condition;
  BlockStatement* consequence;
};

/*
 * Statements
 */
// for code like: let x = 5;
class LetStatement : public Statement {
 public:
  ~LetStatement() {
    delete value;
  }
  void statementNode() { }
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return LetStmt; }

  Token token;  // token LET
  Identifier name;
  Expression* value;
};

// for statement like &a = 6;
class RefStatement : public Statement {
 public:
  ~RefStatement() {
    delete value;
  }
  void statementNode() { }
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return RefStmt; }

  Token token;  // token &
  Identifier name;
  Expression* value;
};

// for code like: return 10;
class ReturnStatement : public Statement {
 public:
  ~ReturnStatement() {
    delete returnValue;
  }
  void statementNode() { }
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ReturnStmt; }

  Token token;  // token RETURN
  Expression* returnValue;
};

// for code like: x + 10;
class ExpressionStatement : public Statement {
 public:
  ~ExpressionStatement() {
    delete expression;
  }
  void statementNode() {}
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return ExprStmt; }

  Token token;  // the first token of the expression
  Expression* expression;
};

class BlockStatement : public Statement {
 public:
  ~BlockStatement() {
    for(auto stmt : statements) {
      delete stmt;
    }
  }
  void statementNode() {}
  std::string TokenLiteral() { return token.literal; }
  std::string String();
  NodeType Type() { return BlockStmt; }

  Token token; // "{"
  std::vector<Statement*> statements;
};

}  // namespace monkey

#endif  // MONKEY_AST_H_