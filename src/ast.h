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
  virtual ~Node() {
  }
};

struct Program : Node {
  vector<unique_ptr<Node>> statements;

  Program(vector<unique_ptr<Node>> statements) : statements(std::move(statements)) {
  }

  void execute(VM *vm) {
    for (auto &stmt : statements) {
      stmt->execute(vm);
    }
  }

  ~Program() {
  }
};

struct Expr : Node {
  virtual Value value() const = 0;
  virtual ~Expr() = default;
};

struct FloatExpr : Expr {
  Value floatValue;

  FloatExpr(float v) : floatValue(v) {
  }

  void execute(VM *vm) {
  }

  Value value() const {
    return floatValue;
  }

  ~FloatExpr() {
  }
};

struct NameExpr : Expr {
  Value v;
  string name;

  NameExpr(string name) : name(name) {
  }

  void execute(VM *vm) {
    v = vm->frames.back().variables[name];
  }

  Value value() const {
    return v;
  }

  ~NameExpr() {
  }
};

struct StringExpr : Expr {
  Value stringValue;

  StringExpr(string s) : stringValue(s) {
  }

  void execute(VM *vm) {
  }

  Value value() const {
    return stringValue;
  }

  ~StringExpr() {
  }
};

enum BinOp {
  Add,
  Sub,
  Div,
  Mul,
  Mod,
  Lt,
  Gt,
  Lte,
  Gte,
  Eq,
};

struct BinOpExpr : Expr {
  BinOp op;
  unique_ptr<Expr> lhs;
  unique_ptr<Expr> rhs;
  Value v;

  BinOpExpr(string opRaw, unique_ptr<Expr> lhs, unique_ptr<Expr> rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) {
    if (opRaw == "+") {
      op = BinOp::Add;
    } else if (opRaw == "-") {
      op = BinOp::Sub;
    } else if (opRaw == "/") {
      op = BinOp::Div;
    } else if (opRaw == "*") {
      op = BinOp::Mul;
    } else if (opRaw == "%") {
      op = BinOp::Mod;
    } else if (opRaw == "<") {
      op = BinOp::Lt;
    } else if (opRaw == ">") {
      op = BinOp::Gt;
    } else if (opRaw == "<=") {
      op = BinOp::Lte;
    } else if (opRaw == ">=") {
      op = BinOp::Gte;
    } else if (opRaw == "==") {
      op = BinOp::Eq;
    } else {
      THROW("Unknown op: %s", opRaw.c_str());
    }
  }

  ~BinOpExpr() {
  }

  void execute(VM *vm) {
    lhs->execute(vm);
    rhs->execute(vm);
    Value lhsVal = lhs->value();
    Value rhsVal = rhs->value();

    switch (op) {
      case BinOp::Add:
        v = lhsVal.add(rhsVal);
        break;
      case BinOp::Sub:
        v = lhsVal.sub(rhsVal);
        break;
      case BinOp::Div:
        v = lhsVal.div(rhsVal);
        break;
      case BinOp::Mul:
        v = lhsVal.mul(rhsVal);
        break;
      case BinOp::Mod:
        v = lhsVal.mod(rhsVal);
        break;
      case BinOp::Lt:
        v = lhsVal.lt(rhsVal);
        break;
      case BinOp::Gt:
        v = rhsVal.lt(lhsVal);
        break;
      case BinOp::Lte:
        v = lhsVal.lte(rhsVal);
        break;
      case BinOp::Gte:
        v = rhsVal.lte(lhsVal);
        break;
      case BinOp::Eq:
        v = lhsVal.eq(rhsVal);
        break;
      default:
        THROW("Unreachable");
    }
  }

  Value value() const {
    return v;
  }
};

struct AssignmentNode : Node {
  unique_ptr<NameExpr> lval;
  unique_ptr<Expr> rval;

  AssignmentNode(unique_ptr<NameExpr> lval, unique_ptr<Expr> rval) : lval(std::move(lval)), rval(std::move(rval)) {
  }

  ~AssignmentNode() {
  }

  void execute(VM *vm) {
    rval->execute(vm);
    vm->frames.back().variables[lval->name] = rval->value();
  }
};

struct LoopNode : Node {
  unique_ptr<Expr> count;
  vector<unique_ptr<Node>> statements;

  LoopNode(unique_ptr<Expr> count, vector<unique_ptr<Node>> statements)
      : count(std::move(count)), statements(std::move(statements)) {
  }

  ~LoopNode() {
  }

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

  IfNode(unique_ptr<Expr> condNode, vector<unique_ptr<Node>> trueStatements, vector<unique_ptr<Node>> falseStatements)
      : condNode(std::move(condNode)),
        trueStatements(std::move(trueStatements)),
        falseStatements(std::move(falseStatements)) {
  }

  ~IfNode() {
  }

  void execute(VM *vm) {
    condNode->execute(vm);
    assert_or_throw(condNode->value().kind == ValueKind::Boolean, "Not bool for IF condition");

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
      : argNames(argNames), statements(std::move(statements)) {
  }

  ~ExecutableFnNode() {
  }

  void execute(VM *vm) {
    for (auto &statement : statements) {
      statement->execute(vm);
    }
  }
};

struct FnDefNode : Node {
  string name;
  shared_ptr<ExecutableFnNode> fn;

  FnDefNode(string name, vector<string> argNames, vector<unique_ptr<Node>> statements) : name(name) {
    fn = make_shared<ExecutableFnNode>(std::move(argNames), std::move(statements));
  }
  ~FnDefNode() {
  }

  void execute(VM *vm) {
    vm->functions[name] = fn;
  }
};

enum FnName {
  FN_FORWARD,
  FN_BACKWARD,
  FN_LEFT,
  FN_RIGHT,
  FN_UP,
  FN_DOWN,
  FN_POS,
  FN_ANGLE,
  FN_THICKNESS,
  FN_RAND,
  FN_CLEAR,
  FN_INTVAR,
  FN_FLOATVAR,
  FN_GETX,
  FN_GETY,
  FN_WINW,
  FN_WINH,
  FN_MIDX,
  FN_MIDY,
  FN_GETANGLE,
  FN_DEBUG,
  FN_PUSH,
  FN_POP,
  FN_LINE,
  FN_UNKNOWN,
};

struct FnCallNode : Expr {
  Value v{};
  FnName knownFnName;
  string fnNameOriginal;
  vector<unique_ptr<Expr>> args;

  FnCallNode(string fnNameOriginal, vector<unique_ptr<Expr>> args)
      : fnNameOriginal(fnNameOriginal), args(std::move(args)) {
    if (fnNameOriginal == "forward" || fnNameOriginal == "f") {
      knownFnName = FnName::FN_FORWARD;
    } else if (fnNameOriginal == "backward" || fnNameOriginal == "b") {
      knownFnName = FnName::FN_BACKWARD;
    } else if (fnNameOriginal == "left" || fnNameOriginal == "l") {
      knownFnName = FnName::FN_LEFT;
    } else if (fnNameOriginal == "right" || fnNameOriginal == "r") {
      knownFnName = FnName::FN_RIGHT;
    } else if (fnNameOriginal == "up" || fnNameOriginal == "u") {
      knownFnName = FnName::FN_UP;
    } else if (fnNameOriginal == "down" || fnNameOriginal == "d") {
      knownFnName = FnName::FN_DOWN;
    } else if (fnNameOriginal == "pos" || fnNameOriginal == "p") {
      knownFnName = FnName::FN_POS;
    } else if (fnNameOriginal == "angle" || fnNameOriginal == "a") {
      knownFnName = FnName::FN_ANGLE;
    } else if (fnNameOriginal == "thickness" || fnNameOriginal == "t") {
      knownFnName = FnName::FN_THICKNESS;
    } else if (fnNameOriginal == "rand") {
      knownFnName = FnName::FN_RAND;
    } else if (fnNameOriginal == "clear" || fnNameOriginal == "c") {
      knownFnName = FnName::FN_CLEAR;
    } else if (fnNameOriginal == "intvar") {
      knownFnName = FnName::FN_INTVAR;
    } else if (fnNameOriginal == "floatvar") {
      knownFnName = FnName::FN_FLOATVAR;
    } else if (fnNameOriginal == "getx") {
      knownFnName = FnName::FN_GETX;
    } else if (fnNameOriginal == "gety") {
      knownFnName = FnName::FN_GETY;
    } else if (fnNameOriginal == "winw") {
      knownFnName = FnName::FN_WINW;
    } else if (fnNameOriginal == "winh") {
      knownFnName = FnName::FN_WINH;
    } else if (fnNameOriginal == "midx") {
      knownFnName = FnName::FN_MIDX;
    } else if (fnNameOriginal == "midy") {
      knownFnName = FnName::FN_MIDY;
    } else if (fnNameOriginal == "getangle") {
      knownFnName = FnName::FN_GETANGLE;
    } else if (fnNameOriginal == "debug") {
      knownFnName = FnName::FN_DEBUG;
    } else if (fnNameOriginal == "push") {
      knownFnName = FnName::FN_PUSH;
    } else if (fnNameOriginal == "pop") {
      knownFnName = FnName::FN_POP;
    } else if (fnNameOriginal == "line") {
      knownFnName = FnName::FN_LINE;
    } else {
      knownFnName = FnName::FN_UNKNOWN;
    }
  }

  void execute(VM *vm) {
    for (auto &arg : args) arg->execute(vm);

    string name;

    switch (knownFnName) {
      case FnName::FN_FORWARD:
        assert_or_throw(args.size() == 1, "Expected 1 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "FORWARD expects a number arg");
        vm->forward(args[0]->value().floatVal);
        break;
      case FnName::FN_BACKWARD:
        assert_or_throw(args.size() == 1, "Expected 1 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "BACKWARD expects a number arg");
        vm->backward(args[0]->value().floatVal);
        break;
      case FnName::FN_LEFT:
        assert_or_throw(args.size() == 1, "Expected 1 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "LEFT expects a number arg");
        vm->left(args[0]->value().floatVal);
        break;
      case FnName::FN_RIGHT:
        assert_or_throw(args.size() == 1, "Expected 1 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "RIGHT expects a number arg");
        vm->right(args[0]->value().floatVal);
        break;
      case FnName::FN_UP:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        vm->isDown = false;
        break;
      case FnName::FN_DOWN:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        vm->isDown = true;
        break;
      case FnName::FN_POS:
        assert_or_throw(args.size() == 2, "Expected 2 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "POS expects number args");
        assert_or_throw(args[1]->value().kind == ValueKind::Number, "POS expects number args");
        vm->setPos(args[0]->value().floatVal, args[1]->value().floatVal);
        break;
      case FnName::FN_ANGLE:
        assert_or_throw(args.size() == 1, "Expected 1 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "POS expects number args");
        vm->angle = args[0]->value().floatVal;
        break;
      case FnName::FN_THICKNESS:
        assert_or_throw(args.size() == 1, "Expected 1 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "THICKNESS expects a number arg");
        vm->thickness = args[0]->value().floatVal;
        break;
      case FnName::FN_RAND:
        assert_or_throw(args.size() == 2, "Expected 2 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "RAND expects a number arg");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "RAND expects a number arg");
        v = Value(randf((int)args[0]->value().floatVal, (int)args[1]->value().floatVal));
        break;
      case FnName::FN_CLEAR:
        vm->reset();
        break;
      case FnName::FN_INTVAR:
        assert_or_throw(args.size() == 4, "Expected 4 args");
        assert_or_throw(args[0]->value().kind == ValueKind::String, "intvar expects a string arg");
        assert_or_throw(args[1]->value().kind == ValueKind::Number, "intvar expects a number arg");
        assert_or_throw(args[2]->value().kind == ValueKind::Number, "intvar expects a number arg");
        assert_or_throw(args[3]->value().kind == ValueKind::Number, "intvar expects a number arg");
        // TODO: This is horrible. Logo and VM is in a circular dep so we cannot
        // fully put Logo structs (non ref / non pointer) into VM.
        name = args[0]->value().strVal;
        vm->intVars[name] = IntVar{(int)args[1]->value().floatVal, (int)args[2]->value().floatVal};
        if (!vm->frames.front().variables.contains(name)) {
          vm->frames.front().variables[name] = args[3]->value();
        }
        break;
      case FnName::FN_FLOATVAR:
        assert_or_throw(args.size() == 4, "Expected 4 args");
        assert_or_throw(args[0]->value().kind == ValueKind::String, "intvar expects a string arg");
        assert_or_throw(args[1]->value().kind == ValueKind::Number, "intvar expects a number arg");
        assert_or_throw(args[2]->value().kind == ValueKind::Number, "intvar expects a number arg");
        assert_or_throw(args[3]->value().kind == ValueKind::Number, "intvar expects a number arg");
        // TODO: This is horrible. Logo and VM is in a circular dep so we cannot
        // fully put Logo structs (non ref / non pointer) into VM.
        name = args[0]->value().strVal;
        vm->floatVars[name] = FloatVar{args[1]->value().floatVal, args[2]->value().floatVal};
        if (!vm->frames.front().variables.contains(name)) {
          vm->frames.front().variables[name] = args[3]->value();
        }
        break;
      case FnName::FN_GETX:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        v = Value(vm->pos.x);
        break;
      case FnName::FN_GETY:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        v = Value(vm->pos.y);
        break;
      case FnName::FN_WINW:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        v = Value((float)(GetScreenWidth()));
        break;
      case FnName::FN_WINH:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        v = Value((float)(GetScreenHeight()));
        break;
      case FnName::FN_MIDX:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        v = Value((float)(GetScreenWidth() >> 1));
        break;
      case FnName::FN_MIDY:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        v = Value((float)(GetScreenHeight() >> 1));
        break;
      case FnName::FN_GETANGLE:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        v = Value(vm->angle);
        break;
      case FnName::FN_DEBUG:
        for (auto &arg : args) {
          arg->execute(vm);
          arg->value().debug();
        }
        break;
      case FnName::FN_PUSH:
        for (auto &arg : args) {
          vm->stack.push_back(arg->value());
        }
        break;
      case FnName::FN_POP:
        assert_or_throw(args.size() == 0, "Expected 0 args");
        assert_or_throw(!vm->stack.empty(), "Empty stack on pop");

        v = vm->stack.back();
        vm->stack.pop_back();
        break;
      case FnName::FN_LINE:
        assert_or_throw(args.size() == 4, "Expected 4 args");
        assert_or_throw(args[0]->value().kind == ValueKind::Number, "intvar expects a number arg");
        assert_or_throw(args[1]->value().kind == ValueKind::Number, "intvar expects a number arg");
        assert_or_throw(args[2]->value().kind == ValueKind::Number, "intvar expects a number arg");
        assert_or_throw(args[3]->value().kind == ValueKind::Number, "intvar expects a number arg");
        vm->history.emplace_back(Vector2{args[0]->value().floatVal, args[1]->value().floatVal},
                                 Vector2{args[2]->value().floatVal, args[3]->value().floatVal}, vm->thickness,
                                 vm->color);
        break;
      default:
        if (!vm->functions.contains(fnNameOriginal)) {
          THROW("Unrecognized function name: %s", fnNameOriginal.c_str());
        }

        auto fn = vm->functions[fnNameOriginal];

        assert_or_throw(args.size() == fn->argNames.size(), "FN arg count mismatch");

        Frame newFrame{};

        for (int i = 0; i < (int)args.size(); i++) {
          newFrame.variables[fn->argNames[i]] = args[i]->value();
        }

        vm->frames.push_back(newFrame);
        fn->execute(vm);
        vm->frames.pop_back();
        break;
    }
  }

  Value value() const {
    return v;
  }

  ~FnCallNode() {
  }
};
}  // namespace Ast
