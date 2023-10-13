// #pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>
#include <vector>

using namespace std;

const vector<string> keywords{"left", "right", "forward", "backward", "up",
                              "down", "fn",    "if",      "else",     "for"};

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

      char next = peek();
      if (isalpha(next)) {
        lexemes.push_back(readWord());
      } else if (isdigit(next)) {
        lexemes.push_back(readNumber());
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
  virtual void execute() {}
};

struct Program : Node {
  vector<Node> statements;

  Program(vector<Node> statements) : statements(statements) {}

  void execute() {}
};

struct Expr : Node {
  void execute() {}
};

struct FnCallNode : Node {
  string fnName;
  vector<Expr> args;

  void execute() {}
};
}  // namespace Ast

struct Parser {
  vector<Lexeme> lexemes;
  size_t ptr = 0;

  Parser(vector<Lexeme> lexemes) : lexemes(lexemes) {}

  Ast::Program parse() {
    vector<Ast::Node> statements;

    while (!isEnd()) {
      statements.push_back(parse_fncall());
    }

    return Ast::Program(statements);
  }

  Ast::FnCallNode parse_fncall() {}

  bool isEnd() const { return ptr >= lexemes.size(); }
  Lexeme peek() const { return lexemes.at(ptr); }
  Lexeme next() { return lexemes.at(ptr++); }
};

int main() {
  cout << "Test parser\n";
  Lexer lexer{"forward(10)"};
  auto lexemes = lexer.parse();
  for (auto lexeme : lexemes) {
    cout << "Lexeme(" << lexemeKindToString(lexeme.kind) << ") = " << lexeme.v
         << endl;
  }
}
