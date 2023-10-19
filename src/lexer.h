#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "util.h"

using namespace std;

const vector<string> keywords{"fn", "if", "else", "loop"};

enum class LexemeKind {
  Keyword,
  Number,
  String,
  Name,
  Semicolon,
  BraceOpen,
  BraceClose,
  ParenOpen,
  ParenClose,
  Comma,
  Op,
};

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
      } else if (c == '"') {
        lexemes.push_back(readString());
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
      } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '<' ||
                 c == '>') {
        string opStr{c};
        lexemes.push_back(Lexeme(LexemeKind::Op, opStr));
        next();
      } else {
        PANIC("Unknown character in lexing: %c", c);
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

  Lexeme readString() {
    assert(next() == '"');
    string s = readWhile([](char c) -> bool { return c != '"'; });
    assert(next() == '"');
    return Lexeme(LexemeKind::String, s);
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
  char peek(unsigned int n) const { return code.at(ptr + n); }
  char next() { return code.at(ptr++); }

  void consumeSpaces() {
    readWhile([](char c) -> bool { return isspace(c); });
  }
};
