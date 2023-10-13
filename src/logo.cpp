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
  Semicolon,
  BraceOpen,
  BraceClose,
  ParenOpen,
  ParenClose,
  Comma,
};

union LexemeData {
  float number;
  string keyword;
};

struct Lexeme {
  LexemeKind kind;
  LexemeData data;

  Lexeme(LexemeKind kind) : kind(kind), data(0.0f) {}
  Lexeme(const Lexeme &) = default;
  ~Lexeme() {}
};

Lexeme makeKeywordLexeme(string keyword) {
  Lexeme out{LexemeKind::Keyword};
  out.data.keyword = keyword;
  return out;
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

    while (!isEnd()) {
      consumeSpaces();
      char next = peek();
      if (isalpha(next)) {
        lexemes.push_back(readWord());
      } else if (isdigit(next)) {
        // lexemes.push_back(readNumber());
      }
    }

    return lexemes;
  }

  Lexeme readWord() {
    string word = readWhile([](char c) -> bool { return isalpha(c); });
    auto it = find(keywords.begin(), keywords.end(), word);
    if (it == keywords.end()) {
      return makeKeywordLexeme(*it);
    }

    exit(EXIT_FAILURE);
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

int main() {
  cout << "Test parser\n";
  Lexer lexer{"forward 10"};

  cout << lexer.readWhile([](char c) -> bool { return isalpha(c); });

  auto lexemes = lexer.parse();
}
