#pragma once

#include <bits/stdc++.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "vm.h"

using namespace std;

const vector<string> keywords{"fn", "if", "else", "for"};

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
      } else if (c == ',') {
        lexemes.push_back(Lexeme(LexemeKind::Comma));
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

struct FnCallNode : Node {
  string fnName;
  vector<unique_ptr<Expr>> args;

  FnCallNode(string fnName, vector<unique_ptr<Expr>> args)
      : fnName(fnName), args(std::move(args)) {}

  void execute(VM *vm) {
    if (fnName == "forward") {
      assert(args.size() == 1);
      vm->forward(args[0]->value());
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
      statements.push_back(parse_fncall());
    }

    return Ast::Program(std::move(statements));
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
    Lexeme val = next();
    assert(val.kind == LexemeKind::Number);

    return make_unique<Ast::FloatExpr>(stof(val.v));
  }

  bool isEnd() const { return ptr >= lexemes.size(); }
  Lexeme peek() const { return lexemes.at(ptr); }
  Lexeme next() { return lexemes.at(ptr++); }
};
