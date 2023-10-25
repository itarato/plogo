#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "lexer.h"
#include "util.h"
#include "value.h"
#include "vm.h"

using namespace std;

namespace Ast {

/** Grammar:

prog       = *statements
statement  = fncall | if | fndef | assignment
fncall     = name popen args pclose
if         = if popen expr pclose bopen statement* bclose [else bopen statement*
bclose]
fndef      = fn name popen args pclose bopen statement* bclose
assignment = name assign expr
args       = expr comma
expr       = number | name | binop | fncall
binop      = number op binop | name op binop

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

struct Expr : Node {
  virtual Value value() = 0;
  virtual ~Expr() {}
};

struct FloatExpr : Expr {
  float v;

  FloatExpr(float v) : v(v) {}

  void execute(VM *vm) {}
  Value value() { return Value(v); }

  ~FloatExpr() {}
};

struct NameExpr : Expr {
  Value v;
  string name;

  NameExpr(string name) : name(name) {}

  void execute(VM *vm) { v = vm->frames.back().variables[name]; }
  Value value() { return v; }

  ~NameExpr() {}
};

struct StringExpr : Expr {
  string s;

  StringExpr(string s) : s(s) {}

  void execute(VM *vm) {}
  Value value() { return Value(s); }

  ~StringExpr() {}
};

struct BinOpExpr : Expr {
  string op;
  unique_ptr<Expr> lhs;
  unique_ptr<Expr> rhs;
  Value v;

  BinOpExpr(string op, unique_ptr<Expr> lhs, unique_ptr<Expr> rhs)
      : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

  ~BinOpExpr() {}

  void execute(VM *vm) {
    lhs->execute(vm);
    rhs->execute(vm);
    Value lhsVal = lhs->value();
    Value rhsVal = rhs->value();

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
    } else if (op == "<=") {
      v = lhsVal.lte(rhsVal);
    } else if (op == ">=") {
      v = rhsVal.lte(lhsVal);
    } else if (op == "==") {
      v = lhsVal.eq(rhsVal);
    } else {
      THROW("Unknown op: %s", op.c_str());
    }
  }
  Value value() { return v; }
};

struct AssignmentNode : Node {
  unique_ptr<NameExpr> lval;
  unique_ptr<Expr> rval;

  AssignmentNode(unique_ptr<NameExpr> lval, unique_ptr<Expr> rval)
      : lval(std::move(lval)), rval(std::move(rval)) {}
  ~AssignmentNode() {}

  void execute(VM *vm) {
    rval->execute(vm);
    vm->frames.back().variables[lval->name] = rval->value();
  }
};

struct LoopNode : Node {
  unique_ptr<Expr> count;
  vector<unique_ptr<Node>> statements;

  LoopNode(unique_ptr<Expr> count, vector<unique_ptr<Node>> statements)
      : count(std::move(count)), statements(std::move(statements)) {}
  ~LoopNode() {}

  void execute(VM *vm) {
    char loopVarNameBuf[8];
    snprintf(loopVarNameBuf, 8, "_i%d", vm->frames.back().loopCount++);
    string loopVarName{loopVarNameBuf};

    count->execute(vm);

    if (count->value().kind != ValueKind::Number) {
      THROW("Only number can be a loop count");
    }

    unsigned int iter = (unsigned int)count->value().floatVal;
    for (unsigned int i = 0; i < iter; i++) {
      vm->frames.back().variables[loopVarName] = Value((float)i);

      for (auto &statement : statements) {
        statement->execute(vm);
      }
    }

    vm->frames.back().loopCount--;
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
    assert_or_throw(condNode->value().kind == ValueKind::Boolean,
                    "Not bool for IF condition");

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

struct FnCallNode : Expr {
  Value v{};
  string fnName;
  vector<unique_ptr<Expr>> args;

  FnCallNode(string fnName, vector<unique_ptr<Expr>> args)
      : fnName(fnName), args(std::move(args)) {}

  void execute(VM *vm) {
    for (auto &arg : args) arg->execute(vm);

    if (fnName == "forward" || fnName == "f") {
      assert_or_throw(args.size() == 1, "Expected FN");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "FORWARD expects a number arg");
      vm->forward(args[0]->value().floatVal);
    } else if (fnName == "backward" || fnName == "b") {
      assert_or_throw(args.size() == 1, "Expected 1 args");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "BACKWARD expects a number arg");
      vm->backward(args[0]->value().floatVal);
    } else if (fnName == "left" || fnName == "l") {
      assert_or_throw(args.size() == 1, "Expected 1 args");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "LEFT expects a number arg");
      vm->left(args[0]->value().floatVal);
    } else if (fnName == "right" || fnName == "r") {
      assert_or_throw(args.size() == 1, "Expected 1 args");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "RIGHT expects a number arg");
      vm->right(args[0]->value().floatVal);
    } else if (fnName == "up" || fnName == "u") {
      assert_or_throw(args.size() == 0, "Expected 0 args");
      vm->isDown = false;
    } else if (fnName == "down" || fnName == "d") {
      assert_or_throw(args.size() == 0, "Expected 0 args");
      vm->isDown = true;
    } else if (fnName == "pos" || fnName == "p") {
      assert_or_throw(args.size() == 2, "Expected 2 args");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "POS expects number args");
      assert_or_throw(args[1]->value().kind == ValueKind::Number,
                      "POS expects number args");
      vm->setPos(args[0]->value().floatVal, args[1]->value().floatVal);
    } else if (fnName == "angle" || fnName == "a") {
      assert_or_throw(args.size() == 1, "Expected 1 args");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "POS expects number args");
      vm->angle = args[0]->value().floatVal;
    } else if (fnName == "thickness" || fnName == "t") {
      assert_or_throw(args.size() == 1, "Expected 1 args");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "THICKNESS expects a number arg");
      vm->thickness = args[0]->value().floatVal;
    } else if (fnName == "rand") {
      assert_or_throw(args.size() == 2, "Expected 2 args");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "RAND expects a number arg");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "RAND expects a number arg");
      v = Value(randf((int)args[0]->value().floatVal,
                      (int)args[1]->value().floatVal));
    } else if (fnName == "clear" || fnName == "c") {
      vm->reset();
    } else if (fnName == "intvar") {
      assert_or_throw(args.size() == 4, "Expected 4 args");
      assert_or_throw(args[0]->value().kind == ValueKind::String,
                      "intvar expects a string arg");
      assert_or_throw(args[1]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      assert_or_throw(args[2]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      assert_or_throw(args[3]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      // TODO: This is horrible. Logo and VM is in a circular dep so we cannot
      // fully put Logo structs (non ref / non pointer) into VM.
      string name{args[0]->value().strVal};
      vm->intVars[name] = IntVar{(int)args[1]->value().floatVal,
                                 (int)args[2]->value().floatVal};
      if (!vm->frames.front().variables.contains(name)) {
        vm->frames.front().variables[name] = args[3]->value();
      }
    } else if (fnName == "floatvar") {
      assert_or_throw(args.size() == 4, "Expected 4 args");
      assert_or_throw(args[0]->value().kind == ValueKind::String,
                      "intvar expects a string arg");
      assert_or_throw(args[1]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      assert_or_throw(args[2]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      assert_or_throw(args[3]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      // TODO: This is horrible. Logo and VM is in a circular dep so we cannot
      // fully put Logo structs (non ref / non pointer) into VM.
      string name{args[0]->value().strVal};
      vm->floatVars[name] =
          FloatVar{args[1]->value().floatVal, args[2]->value().floatVal};
      if (!vm->frames.front().variables.contains(name)) {
        vm->frames.front().variables[name] = args[3]->value();
      }
    } else if (fnName == "getx") {
      assert_or_throw(args.size() == 0, "Expected 0 args");
      v = Value(vm->pos.x);
    } else if (fnName == "gety") {
      assert_or_throw(args.size() == 0, "Expected 0 args");
      v = Value(vm->pos.y);
    } else if (fnName == "midx") {
      assert_or_throw(args.size() == 0, "Expected 0 args");
      v = Value((float)(GetScreenWidth() >> 1));
    } else if (fnName == "midy") {
      assert_or_throw(args.size() == 0, "Expected 0 args");
      v = Value((float)(GetScreenHeight() >> 1));
    } else if (fnName == "getangle") {
      v = Value(vm->angle);
    } else if (fnName == "debug") {
      for (auto &arg : args) {
        arg->execute(vm);
        arg->value().debug();
      }
    } else if (fnName == "push") {
      for (auto &arg : args) {
        vm->stack.push_back(arg->value());
      }
    } else if (fnName == "pop") {
      assert_or_throw(args.size() == 0, "Expected 0 args");
      assert_or_throw(!vm->stack.empty(), "Empty stack on pop");

      v = vm->stack.back();
      vm->stack.pop_back();
    } else if (fnName == "line") {
      assert_or_throw(args.size() == 4, "Expected 4 args");
      assert_or_throw(args[0]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      assert_or_throw(args[1]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      assert_or_throw(args[2]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      assert_or_throw(args[3]->value().kind == ValueKind::Number,
                      "intvar expects a number arg");
      vm->history.emplace_back(
          Vector2{args[0]->value().floatVal, args[1]->value().floatVal},
          Vector2{args[2]->value().floatVal, args[3]->value().floatVal},
          vm->thickness, vm->color);
    } else {
      if (!vm->functions.contains(fnName)) {
        THROW("Unrecognized function name: %s", fnName.c_str());
      }

      auto fn = vm->functions[fnName];

      assert_or_throw(args.size() == fn->argNames.size(),
                      "FN arg count mismatch");

      Frame newFrame{};

      for (int i = 0; i < (int)args.size(); i++) {
        newFrame.variables[fn->argNames[i]] = args[i]->value();
      }

      vm->frames.push_back(newFrame);
      fn->execute(vm);
      vm->frames.pop_back();
    }
  }

  Value value() { return v; }

  ~FnCallNode() {}
};
}  // namespace Ast
