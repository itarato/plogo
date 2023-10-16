#pragma once

#include <bits/stdc++.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "util.h"
#include "vm.h"

using namespace std;

const vector<string> keywords{"fn", "if", "else", "loop"};

enum class LexemeKind {
  Keyword,
  Number,
  Name,
  Semicolon,
  BraceOpen,
  BraceClose,
  ParenOpen,
  ParenClose,
  Comma,
  Op,
};

string lexemeKindToString(LexemeKind kind) {
  switch (kind) {
    case LexemeKind::Keyword:
      return "keyword";
    case LexemeKind::Number:
      return "number";
    case LexemeKind::Name:
      return "name";
    case LexemeKind::ParenOpen:
      return "(";
    case LexemeKind::ParenClose:
      return ")";
    case LexemeKind::Comma:
      return ",";
    default:
      return "unknown";
  }
}

struct Lexeme {
  LexemeKind kind;
  string v{};

  Lexeme(LexemeKind kind) : kind(kind) {}
  Lexeme(LexemeKind kind, string v) : kind(kind), v(v) {}
};

void assert_lexeme(Lexeme lexeme, LexemeKind kind, string v) {
  assert(lexeme.kind == kind);
  assert(lexeme.v == v);
}

struct Lexer {
  string code;
  size_t ptr = 0;

  Lexer(string code) : code(code) {}
  Lexer(const Lexer &code) = delete;
  Lexer(Lexer &&code) = delete;
  ~Lexer() {}

  vector<Lexeme> parse() {
    vector<Lexeme> lexemes{};

    while (true) {
      consumeSpaces();
      if (isEnd()) break;

      char c = peek();
      if (isalpha(c)) {
        lexemes.push_back(readWord());
      } else if (isdigit(c)) {
        lexemes.push_back(readNumber());
      } else if (c == '(') {
        lexemes.push_back(Lexeme(LexemeKind::ParenOpen));
        next();
      } else if (c == ')') {
        lexemes.push_back(Lexeme(LexemeKind::ParenClose));
        next();
      } else if (c == '{') {
        lexemes.push_back(Lexeme(LexemeKind::BraceOpen));
        next();
      } else if (c == '}') {
        lexemes.push_back(Lexeme(LexemeKind::BraceClose));
        next();
      } else if (c == ',') {
        lexemes.push_back(Lexeme(LexemeKind::Comma));
        next();
      } else if (c == '+' || c == '-' || c == '*' || c == '/') {
        string opStr{c};
        lexemes.push_back(Lexeme(LexemeKind::Op, opStr));
        next();
      } else {
        cerr << "Unknown character in lexing\n";
        exit(EXIT_FAILURE);
      }
    }

    return lexemes;
  }

  Lexeme readWord() {
    string word = readWhile([](char c) -> bool { return isalpha(c); });
    auto it = find(keywords.begin(), keywords.end(), word);
    if (it == keywords.end()) {
      return Lexeme(LexemeKind::Name, word);
    } else {
      return Lexeme(LexemeKind::Keyword, word);
    }
  }

  Lexeme readNumber() {
    string word =
        readWhile([](char c) -> bool { return isdigit(c) || c == '.'; });
    return Lexeme(LexemeKind::Number, word);
  }

  string readWhile(bool (*condFn)(char)) {
    string out{};

    while (!isEnd() && condFn(peek())) {
      out.push_back(next());
    }

    return out;
  }

  bool isEnd() const { return ptr >= code.size(); }
  char peek() const { return code.at(ptr); }
  char next() { return code.at(ptr++); }

  void consumeSpaces() {
    readWhile([](char c) -> bool { return isspace(c); });
  }
};

namespace Ast {

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

struct Expr : Node {
  virtual float value() = 0;
  virtual ~Expr() {}
};

struct FloatExpr : Expr {
  float v;

  FloatExpr(float v) : v(v) {}

  void execute(VM *vm) {}
  float value() { return v; }

  ~FloatExpr() {}
};

struct NameExpr : Expr {
  float v;
  string name;

  NameExpr(string name) : name(name) {}

  void execute(VM *vm) { v = vm->frames.back().variables[name]; }
  float value() { return v; }

  ~NameExpr() {}
};

struct BinOpExpr : Expr {
  string op;
  unique_ptr<Expr> lhs;
  unique_ptr<Expr> rhs;
  float v;

  BinOpExpr(string op, unique_ptr<Expr> lhs, unique_ptr<Expr> rhs)
      : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

  ~BinOpExpr() {}

  void execute(VM *vm) {
    lhs->execute(vm);
    rhs->execute(vm);
    float lhsVal = lhs->value();
    float rhsVal = rhs->value();

    if (op == "+") {
      v = lhsVal + rhsVal;
    } else if (op == "-") {
      v = lhsVal - rhsVal;
    } else if (op == "/") {
      v = lhsVal / rhsVal;
    } else if (op == "*") {
      v = lhsVal * rhsVal;
    }
  }
  float value() { return v; }
};

struct LoopNode : Node {
  unique_ptr<Expr> count;
  vector<unique_ptr<Node>> statements;

  LoopNode(unique_ptr<Expr> count, vector<unique_ptr<Node>> statements)
      : count(std::move(count)), statements(std::move(statements)) {}
  ~LoopNode() {}

  void execute(VM *vm) {
    count->execute(vm);
    unsigned int iter = (unsigned int)count->value();
    for (unsigned int i = 0; i < iter; i++) {
      for (auto &statement : statements) {
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
    // UNIMPLEMENTED
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
    if (fnName == "forward" || fnName == "f") {
      assert(args.size() == 1);
      args[0]->execute(vm);
      vm->forward(args[0]->value());
    } else if (fnName == "backward" || fnName == "b") {
      assert(args.size() == 1);
      args[0]->execute(vm);
      vm->backward(args[0]->value());
    } else if (fnName == "left" || fnName == "l") {
      assert(args.size() == 1);
      args[0]->execute(vm);
      vm->left(args[0]->value());
    } else if (fnName == "right" || fnName == "r") {
      assert(args.size() == 1);
      args[0]->execute(vm);
      vm->right(args[0]->value());
    } else if (fnName == "up" || fnName == "u") {
      assert(args.size() == 0);
      vm->isDown = false;
    } else if (fnName == "down" || fnName == "d") {
      assert(args.size() == 0);
      vm->isDown = true;
    } else {
      if (!vm->functions.contains(fnName)) {
        PANIC("Unrecognized function name: %s", fnName.c_str());
      }
      auto fn = vm->functions[fnName];

      assert(args.size() == fn->argNames.size());

      Frame newFrame{};

      for (int i = 0; i < (int)args.size(); i++) {
        args[i]->execute(vm);
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

struct Parser {
  vector<Lexeme> lexemes;
  size_t ptr = 0;

  Parser(vector<Lexeme> lexemes) : lexemes(lexemes) {}

  Ast::Program parse() {
    vector<unique_ptr<Ast::Node>> statements;

    while (!isEnd()) {
      statements.push_back(parse_statement());
    }

    return Ast::Program(std::move(statements));
  }

  unique_ptr<Ast::Node> parse_statement() {
    if (peek().kind == LexemeKind::Keyword && peek().v == "loop") {
      return parse_loop();
    }
    if (peek().kind == LexemeKind::Keyword && peek().v == "fn") {
      return parse_fndef();
    } else {
      return parse_fncall();
    }

    panic("Unkown keyword for statement");
  }

  unique_ptr<Ast::FnDefNode> parse_fndef() {
    assert_lexeme(next(), LexemeKind::Keyword, "fn");

    Lexeme nameToken = next();
    assert(nameToken.kind == LexemeKind::Name);

    assert(next().kind == LexemeKind::ParenOpen);

    vector<string> argNames{};
    while (true) {
      if (peek().kind == LexemeKind::ParenClose) break;

      argNames.push_back(next().v);

      if (peek().kind == LexemeKind::Comma) {
        next();
        continue;
      } else {
        break;
      }
    }

    assert(next().kind == LexemeKind::ParenClose);
    assert(next().kind == LexemeKind::BraceOpen);

    vector<unique_ptr<Ast::Node>> statements;
    while (peek().kind != LexemeKind::BraceClose) {
      statements.push_back(parse_statement());
    }

    assert(next().kind == LexemeKind::BraceClose);

    return make_unique<Ast::FnDefNode>(nameToken.v, argNames,
                                       std::move(statements));
  }

  unique_ptr<Ast::LoopNode> parse_loop() {
    assert_lexeme(next(), LexemeKind::Keyword, "loop");
    assert(next().kind == LexemeKind::ParenOpen);

    unique_ptr<Ast::Expr> count = parse_expr();

    assert(next().kind == LexemeKind::ParenClose);
    assert(next().kind == LexemeKind::BraceOpen);

    vector<unique_ptr<Ast::Node>> statements;
    while (peek().kind != LexemeKind::BraceClose) {
      statements.push_back(parse_statement());
    }

    assert(next().kind == LexemeKind::BraceClose);

    return make_unique<Ast::LoopNode>(std::move(count), std::move(statements));
  }

  unique_ptr<Ast::FnCallNode> parse_fncall() {
    Lexeme nameToken = next();
    assert(nameToken.kind == LexemeKind::Name);
    assert(next().kind == LexemeKind::ParenOpen);

    vector<unique_ptr<Ast::Expr>> args{};
    while (true) {
      if (peek().kind == LexemeKind::ParenClose) break;

      args.push_back(parse_expr());

      if (peek().kind == LexemeKind::Comma) {
        next();
        continue;
      } else {
        break;
      }
    }

    assert(next().kind == LexemeKind::ParenClose);

    return make_unique<Ast::FnCallNode>(nameToken.v, std::move(args));
  }

  unique_ptr<Ast::Expr> parse_expr() {
    vector<unique_ptr<Ast::Expr>> exprList{};
    vector<string> ops{};

    while (true) {
      if (peek().kind == LexemeKind::Number) {
        exprList.push_back(parse_expr_number());
      } else if (peek().kind == LexemeKind::Name) {
        exprList.push_back(parse_expr_name());
      }

      if (peek().kind == LexemeKind::Op) {
        ops.push_back(next().v);
      } else {
        break;
      }
    }

    assert(exprList.size() == ops.size() + 1);

    for (auto &op : ops) {
      auto rhs = std::move(exprList.back());
      exprList.pop_back();
      auto lhs = std::move(exprList.back());
      exprList.pop_back();

      unique_ptr<Ast::Expr> newExpr =
          make_unique<Ast::BinOpExpr>(op, std::move(lhs), std::move(rhs));
      exprList.push_back(std::move(newExpr));
    }

    assert(exprList.size() == 1);
    return std::move(exprList.back());
  }

  unique_ptr<Ast::Expr> parse_expr_number() {
    Lexeme val = next();
    assert(val.kind == LexemeKind::Number);
    return make_unique<Ast::FloatExpr>(stof(val.v));
  }

  unique_ptr<Ast::Expr> parse_expr_name() {
    Lexeme val = next();
    assert(val.kind == LexemeKind::Name);
    return make_unique<Ast::NameExpr>(val.v);
  }

  bool isEnd() const { return ptr >= lexemes.size(); }
  Lexeme peek() const { return lexemes.at(ptr); }
  Lexeme next() { return lexemes.at(ptr++); }
};
