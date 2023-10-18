#pragma once

#include <bits/stdc++.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "lexer.h"
#include "util.h"
#include "vm.h"

using namespace std;

namespace Ast {

struct ExprValue;

/**

prog      = *statements
statement = fncall
fncall    = name popen args pclose
args      = *expr comma

**/

struct Node {
  virtual void execute(VM *vm) = 0;
  virtual ~Node() {}
};

struct Program : Node {
  vector<unique_ptr<Node>> statements;

  Program(vector<unique_ptr<Node>> statements)
      : statements(std::move(statements)) {}

  void execute(VM *vm) {
    for (auto &stmt : statements) {
      stmt->execute(vm);
    }
  }

  ~Program() {}
};

enum class ExprValueKind {
  String,
  Number,
  Boolean,
  Unknown,
};

struct ExprValue {
  union {
    float floatVal;
    bool boolVal;
    string strVal;
  };
  ExprValueKind kind;

  ExprValue() : floatVal(0.0), kind(ExprValueKind::Unknown) {}
  ExprValue(float v) : floatVal(v), kind(ExprValueKind::Number) {}
  ExprValue(bool v) : boolVal(v), kind(ExprValueKind::Boolean) {}
  ExprValue(string v) : strVal(v), kind(ExprValueKind::String) {}

  ExprValue &operator=(const ExprValue &other) {
    kind = other.kind;
    switch (other.kind) {
      case ExprValueKind::Boolean:
        boolVal = other.boolVal;
        break;
      case ExprValueKind::Number:
        floatVal = other.floatVal;
        break;
      case ExprValueKind::String:
        strVal = other.strVal;
        break;
      default:
        floatVal = 0.0f;
        break;
    };

    return *this;
  }

  ExprValue(const ExprValue &other) { *this = other; }

  ~ExprValue() {
    if (kind == ExprValueKind::String) {
      strVal.~string();
    }
  }

  ExprValue add(ExprValue &other) {
    if (!is_same_kind(other, ExprValueKind::Number)) {
      panic("Cannot sum non number values");
    }

    return ExprValue(floatVal + other.floatVal);
  }

  ExprValue sub(ExprValue &other) {
    if (!is_same_kind(other, ExprValueKind::Number)) {
      panic("Cannot subtract non number values");
    }

    return ExprValue(floatVal - other.floatVal);
  }

  ExprValue mul(ExprValue &other) {
    if (!is_same_kind(other, ExprValueKind::Number)) {
      panic("Cannot multiply non number values");
    }

    return ExprValue(floatVal * other.floatVal);
  }

  ExprValue div(ExprValue &other) {
    if (!is_same_kind(other, ExprValueKind::Number)) {
      panic("Cannot divide non number values");
    }

    return ExprValue(floatVal / other.floatVal);
  }

  ExprValue lt(ExprValue &other) {
    if (!is_same_kind(other, ExprValueKind::Number)) {
      panic("Cannot compare non number values");
    }

    return ExprValue(floatVal < other.floatVal);
  }

  inline bool is_same_kind(ExprValue &other, ExprValueKind assertedKind) {
    return kind == assertedKind && other.kind == assertedKind;
  }
};

struct Expr : Node {
  virtual ExprValue value() = 0;
  virtual ~Expr() {}
};

struct FloatExpr : Expr {
  float v;

  FloatExpr(float v) : v(v) {}

  void execute(VM *vm) {}
  ExprValue value() { return ExprValue(v); }

  ~FloatExpr() {}
};

struct NameExpr : Expr {
  ExprValue v;
  string name;

  NameExpr(string name) : name(name) {}

  void execute(VM *vm) { v = vm->frames.back().variables[name]; }
  ExprValue value() { return v; }

  ~NameExpr() {}
};

struct StringExpr : Expr {
  string s;

  StringExpr(string s) : s(s) {}

  void execute(VM *vm) {}
  ExprValue value() { return ExprValue(s); }

  ~StringExpr() {}
};

struct BinOpExpr : Expr {
  string op;
  unique_ptr<Expr> lhs;
  unique_ptr<Expr> rhs;
  ExprValue v;

  BinOpExpr(string op, unique_ptr<Expr> lhs, unique_ptr<Expr> rhs)
      : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

  ~BinOpExpr() {}

  void execute(VM *vm) {
    lhs->execute(vm);
    rhs->execute(vm);
    ExprValue lhsVal = lhs->value();
    ExprValue rhsVal = rhs->value();

    if (op == "+") {
      v = lhsVal.add(rhsVal);
    } else if (op == "-") {
      v = lhsVal.sub(rhsVal);
    } else if (op == "/") {
      v = lhsVal.div(rhsVal);
    } else if (op == "*") {
      v = lhsVal.mul(rhsVal);
    } else if (op == "<") {
      v = lhsVal.lt(rhsVal);
    } else if (op == ">") {
      v = rhsVal.lt(lhsVal);
    } else {
      PANIC("Unknown op: %s", op.c_str());
    }
  }
  ExprValue value() { return v; }
};

struct LoopNode : Node {
  unique_ptr<Expr> count;
  vector<unique_ptr<Node>> statements;

  LoopNode(unique_ptr<Expr> count, vector<unique_ptr<Node>> statements)
      : count(std::move(count)), statements(std::move(statements)) {}
  ~LoopNode() {}

  void execute(VM *vm) {
    count->execute(vm);

    if (count->value().kind != ExprValueKind::Number) {
      panic("Only number can be a loop count");
    }

    unsigned int iter = (unsigned int)count->value().floatVal;
    for (unsigned int i = 0; i < iter; i++) {
      for (auto &statement : statements) {
        statement->execute(vm);
      }
    }
  }
};

struct IfNode : Node {
  unique_ptr<Expr> condNode;
  vector<unique_ptr<Node>> trueStatements;
  vector<unique_ptr<Node>> falseStatements;

  IfNode(unique_ptr<Expr> condNode, vector<unique_ptr<Node>> trueStatements,
         vector<unique_ptr<Node>> falseStatements)
      : condNode(std::move(condNode)),
        trueStatements(std::move(trueStatements)),
        falseStatements(std::move(falseStatements)) {}
  ~IfNode() {}

  void execute(VM *vm) {
    condNode->execute(vm);
    assert(condNode->value().kind == ExprValueKind::Boolean);

    if (condNode->value().boolVal) {
      for (auto &statement : trueStatements) {
        statement->execute(vm);
      }
    } else {
      for (auto &statement : falseStatements) {
        statement->execute(vm);
      }
    }
  }
};

struct ExecutableFnNode : Node {
  vector<string> argNames;
  vector<unique_ptr<Node>> statements;

  ExecutableFnNode(vector<string> argNames, vector<unique_ptr<Node>> statements)
      : argNames(argNames), statements(std::move(statements)) {}
  ~ExecutableFnNode() {}

  void execute(VM *vm) {
    for (auto &statement : statements) {
      statement->execute(vm);
    }
  }
};

struct FnDefNode : Node {
  string name;
  shared_ptr<ExecutableFnNode> fn;

  FnDefNode(string name, vector<string> argNames,
            vector<unique_ptr<Node>> statements)
      : name(name) {
    fn = make_shared<ExecutableFnNode>(std::move(argNames),
                                       std::move(statements));
  }
  ~FnDefNode() {}

  void execute(VM *vm) { vm->functions[name] = fn; }
};

struct FnCallNode : Node {
  string fnName;
  vector<unique_ptr<Expr>> args;

  FnCallNode(string fnName, vector<unique_ptr<Expr>> args)
      : fnName(fnName), args(std::move(args)) {}

  void execute(VM *vm) {
    for (auto &arg : args) arg->execute(vm);

    if (fnName == "forward" || fnName == "f") {
      assert(args.size() == 1);
      assert(args[0]->value().kind == ExprValueKind::Number);
      vm->forward(args[0]->value().floatVal);
    } else if (fnName == "backward" || fnName == "b") {
      assert(args.size() == 1);
      assert(args[0]->value().kind == ExprValueKind::Number);
      vm->backward(args[0]->value().floatVal);
    } else if (fnName == "left" || fnName == "l") {
      assert(args.size() == 1);
      assert(args[0]->value().kind == ExprValueKind::Number);
      vm->left(args[0]->value().floatVal);
    } else if (fnName == "right" || fnName == "r") {
      assert(args.size() == 1);
      assert(args[0]->value().kind == ExprValueKind::Number);
      vm->right(args[0]->value().floatVal);
    } else if (fnName == "up" || fnName == "u") {
      assert(args.size() == 0);
      vm->isDown = false;
    } else if (fnName == "down" || fnName == "d") {
      assert(args.size() == 0);
      vm->isDown = true;
    } else if (fnName == "pos" || fnName == "p") {
      assert(args.size() == 2);
      assert(args[0]->value().kind == ExprValueKind::Number);
      assert(args[1]->value().kind == ExprValueKind::Number);
      vm->setPos(args[0]->value().floatVal, args[1]->value().floatVal);
    } else if (fnName == "thickness" || fnName == "thick" || fnName == "t") {
      assert(args.size() == 1);
      assert(args[0]->value().kind == ExprValueKind::Number);
      vm->thickness = args[0]->value().floatVal;
    } else if (fnName == "intvar") {
      assert(args.size() == 4);
      assert(args[0]->value().kind == ExprValueKind::String);
      assert(args[1]->value().kind == ExprValueKind::Number);
      assert(args[2]->value().kind == ExprValueKind::Number);
      assert(args[3]->value().kind == ExprValueKind::Number);
      // TODO: This is horrible. Logo and VM is in a circular dep so we cannot
      // fully put Logo structs (non ref / non pointer) into VM.
      string name{args[0]->value().strVal};
      vm->intVars[name] = IntVar{(int)args[1]->value().floatVal,
                                 (int)args[2]->value().floatVal};
      if (!vm->frames.front().variables.contains(name)) {
        vm->frames.front().variables[name] = args[3]->value();
      }
    } else {
      if (!vm->functions.contains(fnName)) {
        PANIC("Unrecognized function name: %s", fnName.c_str());
      }

      auto fn = vm->functions[fnName];

      assert(args.size() == fn->argNames.size());

      Frame newFrame{};

      for (int i = 0; i < (int)args.size(); i++) {
        newFrame.variables[fn->argNames[i]] = args[i]->value();
      }

      vm->frames.push_back(newFrame);
      fn->execute(vm);
      vm->frames.pop_back();
    }
  }

  ~FnCallNode() {}
};
}  // namespace Ast
