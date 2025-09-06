#pragma once

#include <exception>
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
  Assignment,
};

int precedence(string s) {
  if (s == "<" || s == ">" || s == "<=" || s == ">=" || s == "==") {
    return 3;
  } else if (s == "+" || s == "-") {
    return 2;
  } else if (s == "*" || s == "/" || s == "%") {
    return 1;
  }

  THROW("Unexpected op in precedence check: %s", s.c_str());
  return -1;  // Unreachable, to satisfy return expectation.
}

struct Lexeme {
  LexemeKind kind;
  string v{};

  Lexeme(LexemeKind kind) : kind(kind) {
  }
  Lexeme(LexemeKind kind, string v) : kind(kind), v(v) {
  }
};

struct Lexer {
  string code;
  size_t ptr = 0;

  Lexer(string code) : code(code) {
  }
  Lexer(const Lexer &code) = delete;
  Lexer(Lexer &&code) = delete;
  ~Lexer() {
  }

  vector<Lexeme> parse() {
    vector<Lexeme> lexemes{};

    while (true) {
      consumeSpaces();
      if (isEnd()) break;

      char c = peek();
      if (isalpha(c) || c == '_') {
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
      } else if (c == '+' || c == '*' || c == '/' || c == '%') {
        string opStr{c};
        lexemes.push_back(Lexeme(LexemeKind::Op, opStr));
        next();
      } else if (c == '-') {
        next();

        if (isdigit(peek())) {
          lexemes.push_back(readNumber(true));
        } else {
          string opStr{c};
          lexemes.push_back(Lexeme(LexemeKind::Op, opStr));
        }
      } else if (c == '<' || c == '>') {
        next();
        string opStr{c};
        if (peek() == '=') opStr.push_back(next());
        lexemes.push_back(Lexeme(LexemeKind::Op, opStr));
      } else if (c == '=') {
        next();

        if (peek() == '=') {
          next();
          lexemes.push_back(Lexeme(LexemeKind::Op, "=="));
        } else {
          lexemes.push_back(Lexeme(LexemeKind::Assignment));
        }
      } else if (c == '#') {
        readWhile([](char c) -> bool { return c != '\n'; });
      } else {
        THROW("Unknown character in lexing <%c> at pos %d", c, ptr);
      }
    }

    return lexemes;
  }

  Lexeme readWord() {
    string word = readWhile([](char c) -> bool { return isalnum(c) || c == '_'; });
    auto it = find(keywords.begin(), keywords.end(), word);
    if (it == keywords.end()) {
      return Lexeme(LexemeKind::Name, word);
    } else {
      return Lexeme(LexemeKind::Keyword, word);
    }
  }

  Lexeme readNumber(bool negative = false) {
    string word;

    if (negative) {
      word = "-";
    } else {
      word = "";
    }

    word += readWhile([](char c) -> bool { return isdigit(c) || c == '.'; });
    return Lexeme(LexemeKind::Number, word);
  }

  Lexeme readString() {
    assert_or_throw(next() == '"', "Expected opening double quote");
    string s = readWhile([](char c) -> bool { return c != '"'; });
    assert_or_throw(next() == '"', "Expected closing double quote");
    return Lexeme(LexemeKind::String, s);
  }

  string readWhile(bool (*condFn)(char)) {
    string out{};

    while (!isEnd() && condFn(peek())) {
      out.push_back(next());
    }

    return out;
  }

  bool isEnd() const {
    return ptr >= code.size();
  }
  char peek() const {
    if (isEnd()) {
      throw runtime_error("EOF when asking peek in lexer");
    }
    return code.at(ptr);
  }
  char peek(unsigned int n) const {
    if (isEnd()) {
      throw runtime_error("EOF when asking peek-n in lexer");
    }
    return code.at(ptr + n);
  }
  char next() {
    if (isEnd()) {
      throw runtime_error("EOF when asking next char in lexer");
    }
    return code.at(ptr++);
  }

  void consumeSpaces() {
    readWhile([](char c) -> bool { return isspace(c); });
  }
};
