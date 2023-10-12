// #pragma once

#include <iostream>
#include <vector>

using namespace std;

const char *keywords[] = {"left", "right", "forward", "backward", "up",
                          "down", "fn",    "if",      "else",     "for"};

enum class LexemeKind {
  Keyword,
  Number,
  Semicolon,
  BraceOpen,
  BraceClose,
  ParenOpen,
  ParenClose,
  Comma,
};

struct Lexeme {
  union {
    float number;
  } data;
  LexemeKind kind;
};

struct Lexer {
  string code;

  Lexer(string code) : code(code) {}
  Lexer(const Lexer &code) = delete;
  Lexer(Lexer &&code) = delete;
  ~Lexer() {}

  vector<Lexeme> parse() const {
    vector<Lexeme> lexemes{};
    return lexemes;
  }
};

int main() {}
