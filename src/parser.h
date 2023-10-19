#pragma once

#include <exception>
#include <vector>

#include "ast.h"

using namespace std;

void assert_lexeme(Lexeme lexeme, LexemeKind kind, string v) {
  char msgbuf[128];

  if (lexeme.kind != kind) {
    sprintf(msgbuf, "Lexeme mismatch. Expected kind %d but got: %d",
            static_cast<int>(kind), static_cast<int>(lexeme.kind));
    throw runtime_error(msgbuf);
  }

  if (lexeme.v != v) {
    sprintf(msgbuf, "Lexeme mismatch. Expected value: %s but got: %s",
            v.c_str(), lexeme.v.c_str());
    throw runtime_error(msgbuf);
  }
}

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
    } else if (peek().kind == LexemeKind::Keyword && peek().v == "fn") {
      return parse_fndef();
    } else if (peek().kind == LexemeKind::Keyword && peek().v == "if") {
      return parse_if();
    } else {
      return parse_fncall();
    }
  }

  unique_ptr<Ast::IfNode> parse_if() {
    assert_lexeme(next(), LexemeKind::Keyword, "if");
    assert_lexeme(next().kind, LexemeKind::ParenOpen, "");

    unique_ptr<Ast::Expr> condExpr = parse_expr();

    assert_lexeme(next().kind, LexemeKind::ParenClose, "");
    assert_lexeme(next().kind, LexemeKind::BraceOpen, "");

    vector<unique_ptr<Ast::Node>> trueStatements{};
    while (peek().kind != LexemeKind::BraceClose) {
      trueStatements.push_back(parse_statement());
    }

    assert_lexeme(next().kind, LexemeKind::BraceClose, "");

    vector<unique_ptr<Ast::Node>> falseStatements{};
    if (peek().kind == LexemeKind::Keyword && peek().v == "else") {
      assert_lexeme(next(), LexemeKind::Keyword, "else");
      assert_lexeme(next().kind, LexemeKind::BraceOpen, "");

      while (peek().kind != LexemeKind::BraceClose) {
        falseStatements.push_back(parse_statement());
      }

      assert_lexeme(next().kind, LexemeKind::BraceClose, "");
    }

    return make_unique<Ast::IfNode>(std::move(condExpr),
                                    std::move(trueStatements),
                                    std::move(falseStatements));
  }

  unique_ptr<Ast::FnDefNode> parse_fndef() {
    assert_lexeme(next(), LexemeKind::Keyword, "fn");

    Lexeme nameToken = next();
    assert_lexeme(nameToken.kind, LexemeKind::Name, "");

    assert_lexeme(next().kind, LexemeKind::ParenOpen, "");

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

    assert_lexeme(next().kind, LexemeKind::ParenClose, "");
    assert_lexeme(next().kind, LexemeKind::BraceOpen, "");

    vector<unique_ptr<Ast::Node>> statements;
    while (peek().kind != LexemeKind::BraceClose) {
      statements.push_back(parse_statement());
    }

    assert_lexeme(next().kind, LexemeKind::BraceClose, "");

    return make_unique<Ast::FnDefNode>(nameToken.v, argNames,
                                       std::move(statements));
  }

  unique_ptr<Ast::LoopNode> parse_loop() {
    assert_lexeme(next(), LexemeKind::Keyword, "loop");
    assert_lexeme(next().kind, LexemeKind::ParenOpen, "");

    unique_ptr<Ast::Expr> count = parse_expr();

    assert_lexeme(next().kind, LexemeKind::ParenClose, "");
    assert_lexeme(next().kind, LexemeKind::BraceOpen, "");

    vector<unique_ptr<Ast::Node>> statements{};
    while (peek().kind != LexemeKind::BraceClose) {
      statements.push_back(parse_statement());
    }

    assert_lexeme(next().kind, LexemeKind::BraceClose, "");

    return make_unique<Ast::LoopNode>(std::move(count), std::move(statements));
  }

  unique_ptr<Ast::FnCallNode> parse_fncall() {
    Lexeme nameToken = next();
    assert_lexeme(nameToken.kind, LexemeKind::Name, "");
    assert_lexeme(next().kind, LexemeKind::ParenOpen, "");

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

    assert_lexeme(next().kind, LexemeKind::ParenClose, "");

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
      } else if (peek().kind == LexemeKind::String) {
        exprList.push_back(parse_expr_string());
      } else {
        throw runtime_error("Unexpected lexeme kind for expression");
      }

      if (peek().kind == LexemeKind::Op) {
        ops.push_back(next().v);
      } else {
        break;
      }
    }

    assert_or_throw(exprList.size() == ops.size() + 1,
                    "Operator and operand counts does not align");

    for (auto& op : ops) {
      auto rhs = std::move(exprList.back());
      exprList.pop_back();
      auto lhs = std::move(exprList.back());
      exprList.pop_back();

      unique_ptr<Ast::Expr> newExpr =
          make_unique<Ast::BinOpExpr>(op, std::move(lhs), std::move(rhs));
      exprList.push_back(std::move(newExpr));
    }

    assert_or_throw(exprList.size() == 1, "Expected 1 operand to stay");
    return std::move(exprList.back());
  }

  unique_ptr<Ast::Expr> parse_expr_number() {
    Lexeme val = next();
    assert_lexeme(val.kind, LexemeKind::Number, "");
    return make_unique<Ast::FloatExpr>(stof(val.v));
  }

  unique_ptr<Ast::Expr> parse_expr_name() {
    Lexeme val = next();
    assert_lexeme(val.kind, LexemeKind::Name, "");
    return make_unique<Ast::NameExpr>(val.v);
  }

  unique_ptr<Ast::Expr> parse_expr_string() {
    Lexeme val = next();
    assert_lexeme(val.kind, LexemeKind::String, "");
    return make_unique<Ast::StringExpr>(val.v);
  }

  bool isEnd() const { return ptr >= lexemes.size(); }
  Lexeme peek() const {
    if (isEnd()) {
      throw runtime_error("EOF when peeking lexeme");
    }
    return lexemes.at(ptr);
  }
  Lexeme next() {
    if (isEnd()) {
      throw runtime_error("EOF when asking next lexeme");
    }
    return lexemes.at(ptr++);
  }
};
