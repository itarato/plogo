#pragma once

#include <vector>

#include "ast.h"

using namespace std;

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

    panic("Unkown keyword for statement");
  }

  unique_ptr<Ast::IfNode> parse_if() {
    assert_lexeme(next(), LexemeKind::Keyword, "if");
    assert(next().kind == LexemeKind::ParenOpen);

    unique_ptr<Ast::Expr> condExpr = parse_expr();

    assert(next().kind == LexemeKind::ParenClose);
    assert(next().kind == LexemeKind::BraceOpen);

    vector<unique_ptr<Ast::Node>> trueStatements{};
    while (peek().kind != LexemeKind::BraceClose) {
      trueStatements.push_back(parse_statement());
    }

    assert(next().kind == LexemeKind::BraceClose);

    vector<unique_ptr<Ast::Node>> falseStatements{};
    if (peek().kind == LexemeKind::Keyword && peek().v == "else") {
      assert_lexeme(next(), LexemeKind::Keyword, "else");
      assert(next().kind == LexemeKind::BraceOpen);

      while (peek().kind != LexemeKind::BraceClose) {
        falseStatements.push_back(parse_statement());
      }

      assert(next().kind == LexemeKind::BraceClose);
    }

    return make_unique<Ast::IfNode>(std::move(condExpr),
                                    std::move(trueStatements),
                                    std::move(falseStatements));
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

    vector<unique_ptr<Ast::Node>> statements{};
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
      } else if (peek().kind == LexemeKind::String) {
        exprList.push_back(parse_expr_string());
      } else {
        PANIC("Unexpected lexeme kind for expression");
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

  unique_ptr<Ast::Expr> parse_expr_string() {
    Lexeme val = next();
    assert(val.kind == LexemeKind::String);
    return make_unique<Ast::StringExpr>(val.v);
  }

  bool isEnd() const { return ptr >= lexemes.size(); }
  Lexeme peek() const { return lexemes.at(ptr); }
  Lexeme next() { return lexemes.at(ptr++); }
};
