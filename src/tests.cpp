#include <iostream>
#include <utility>

#include "ast.h"
#include "util.h"
#include "vm.h"

using namespace std;

void test_tokens(string code, vector<pair<LexemeKind, string>> expected) {
  Lexer lexer{code};
  auto lexemes = lexer.parse();

  if (lexemes.size() != expected.size()) {
    FAIL("Unequal lexeme size %d != %d", expected.size(), lexemes.size());
  }

  for (int i = 0; i < (int)expected.size(); i++) {
    if (expected[i].first != lexemes[i].kind) {
      FAIL("Lexeme kind mismatch %d != %d", expected[i].first, lexemes[i].kind);
    }
    if (expected[i].second != lexemes[i].v) {
      FAIL("Lexeme value mismatch %s != %s", expected[i].second.c_str(),
           lexemes[i].v.c_str());
    }
  }

  PASS("test_tokens");
}

void test_vm(string code, void (*testFn)(VM*)) {
  Lexer lexer{code};
  Parser parser{lexer.parse()};
  Ast::Program prg = parser.parse();
  VM vm{};
  prg.execute(&vm);

  testFn(&vm);
}

int main() {
  INFO("start");

  test_tokens("forward(10)", {{LexemeKind::Name, "forward"},
                              {LexemeKind::ParenOpen, ""},
                              {LexemeKind::Number, "10"},
                              {LexemeKind::ParenClose, ""}});

  test_tokens("loop(12) { b(10.5) }", {{LexemeKind::Keyword, "loop"},
                                       {LexemeKind::ParenOpen, ""},
                                       {LexemeKind::Number, "12"},
                                       {LexemeKind::ParenClose, ""},
                                       {LexemeKind::BraceOpen, ""},
                                       {LexemeKind::Name, "b"},
                                       {LexemeKind::ParenOpen, ""},
                                       {LexemeKind::Number, "10.5"},
                                       {LexemeKind::ParenClose, ""},
                                       {LexemeKind::BraceClose, ""}});

  test_tokens("10 + a", {
                            {LexemeKind::Number, "10"},
                            {LexemeKind::Op, "+"},
                            {LexemeKind::Name, "a"},
                        });

  test_tokens("\"abc\"", {{LexemeKind::String, "abc"}});

  test_tokens("fn circle(iter, size) { f(size) r(360 / iter) }",
              {
                  {LexemeKind::Keyword, "fn"},  {LexemeKind::Name, "circle"},
                  {LexemeKind::ParenOpen, ""},  {LexemeKind::Name, "iter"},
                  {LexemeKind::Comma, ""},      {LexemeKind::Name, "size"},
                  {LexemeKind::ParenClose, ""}, {LexemeKind::BraceOpen, ""},
                  {LexemeKind::Name, "f"},      {LexemeKind::ParenOpen, ""},
                  {LexemeKind::Name, "size"},   {LexemeKind::ParenClose, ""},
                  {LexemeKind::Name, "r"},      {LexemeKind::ParenOpen, ""},
                  {LexemeKind::Number, "360"},  {LexemeKind::Op, "/"},
                  {LexemeKind::Name, "iter"},   {LexemeKind::ParenClose, ""},
                  {LexemeKind::BraceClose, ""},
              });

  test_tokens("a > 1", {
                           {LexemeKind::Name, "a"},
                           {LexemeKind::Op, ">"},
                           {LexemeKind::Number, "1"},
                       });

  test_vm("forward(10)", [](VM* vm) {
    ASSERT(eqf(vm->angle, 0.0), "angle is 0");
    ASSERT(eqf(vm->pos.x, 0.0), "x is 0.0");
    ASSERT(eqf(vm->pos.y, -10.0), "y is -10.0");
  });

  test_vm("right(90)", [](VM* vm) {
    ASSERT(eqf(vm->angle, 90.0), "angle is 90");
    ASSERT(eqf(vm->pos.x, 0.0), "x is 0.0");
    ASSERT(eqf(vm->pos.y, 0.0), "y is 0.0");
  });

  test_vm("loop(3) { f(100) r(90) }", [](VM* vm) {
    ASSERT(eqf(vm->angle, 270.0), "angle is 270");
    ASSERT(eqf(vm->pos.x, 100.0), "x is 100.0");
    ASSERT(eqf(vm->pos.y, 0.0), "y is 0.0");
  });

  test_vm("forward(10 + 20)",
          [](VM* vm) { ASSERT(eqf(vm->pos.y, -30.0), "y is -30.0"); });

  test_vm("forward(20 - 5)",
          [](VM* vm) { ASSERT(eqf(vm->pos.y, -15.0), "y is -15.0"); });

  test_vm("fn walk(x) { f(x) } walk(10)",
          [](VM* vm) { ASSERT(eqf(vm->pos.y, -10.0), "y is -10.0"); });

  test_vm("if (1.5 < 3.0) { f(10) } else { f(20) }",
          [](VM* vm) { ASSERT(eqf(vm->pos.y, -10.0), "y is -10.0"); });

  test_vm("if (1.5 > 3.0) { f(10) } else { f(20) }",
          [](VM* vm) { ASSERT(eqf(vm->pos.y, -20.0), "y is -20.0"); });

  PASS("all");
}
