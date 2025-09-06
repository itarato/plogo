#include <iostream>
#include <utility>

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"
#include "value.h"
#include "vm.h"

#define FAIL(...) log_and_fail("\x1b[91mFAIL\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)
#define PASS(...) log("\x1b[92mPASS\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)
#define ASSERT(exp, msg) (exp ? PASS(msg) : FAIL(msg))

static int failCount = 0;

void log_and_fail(const char* level, const char* fileName, int lineNo, const char* s, ...) {
  va_list args;
  va_start(args, s);

  log_va_list(level, fileName, lineNo, s, args);

  va_end(args);

  failCount++;
}

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
      FAIL("Lexeme value mismatch %s != %s", expected[i].second.c_str(), lexemes[i].v.c_str());
    }
  }

  PASS("test_tokens: %s", code.c_str());
}

void test_vm(string code, void (*testFn)(VM*)) {
  Lexer lexer{code};
  Parser parser{lexer.parse()};
  Ast::Program prg = parser.parse();
  VM vm{};
  prg.execute(&vm);

  testFn(&vm);
}

void test_vm_raise(string code) {
  bool gotRaise{false};

  try {
    Lexer lexer{code};
    Parser parser{lexer.parse()};
    Ast::Program prg = parser.parse();
    VM vm{};
    prg.execute(&vm);
  } catch (runtime_error& e) {
    gotRaise = true;
    PASS("Code: \"%s\" raised the expected exception", code.c_str());
  } catch (...) {
    FAIL("Code: \"%s\" did not raise the expected exception", code.c_str());
  }

  if (!gotRaise) {
    FAIL("Code: \"%s\" did not raise the expected exception", code.c_str());
  }
}

struct TestValueMock {
  Value v;

  TestValueMock(string s) : v(s) {
  }

  Value get() {
    return v;
  }
};

void test_value() {
  string s{"hello"};
  TestValueMock tvm{s};

  tvm = tvm;

  auto x = tvm.get();

  Value v{string{"world"}};
  v = v;

  PASS("Value with string works: %s %s", tvm.v.strVal, v.strVal);
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
                  {LexemeKind::Keyword, "fn"},  {LexemeKind::Name, "circle"}, {LexemeKind::ParenOpen, ""},
                  {LexemeKind::Name, "iter"},   {LexemeKind::Comma, ""},      {LexemeKind::Name, "size"},
                  {LexemeKind::ParenClose, ""}, {LexemeKind::BraceOpen, ""},  {LexemeKind::Name, "f"},
                  {LexemeKind::ParenOpen, ""},  {LexemeKind::Name, "size"},   {LexemeKind::ParenClose, ""},
                  {LexemeKind::Name, "r"},      {LexemeKind::ParenOpen, ""},  {LexemeKind::Number, "360"},
                  {LexemeKind::Op, "/"},        {LexemeKind::Name, "iter"},   {LexemeKind::ParenClose, ""},
                  {LexemeKind::BraceClose, ""},
              });

  test_tokens("a > 1", {
                           {LexemeKind::Name, "a"},
                           {LexemeKind::Op, ">"},
                           {LexemeKind::Number, "1"},
                       });

  test_tokens("a = 1", {{LexemeKind::Name, "a"}, {LexemeKind::Assignment, ""}, {LexemeKind::Number, "1"}});

  test_tokens("+ - * / < > <= >= ==", {
                                          {LexemeKind::Op, "+"},
                                          {LexemeKind::Op, "-"},
                                          {LexemeKind::Op, "*"},
                                          {LexemeKind::Op, "/"},
                                          {LexemeKind::Op, "<"},
                                          {LexemeKind::Op, ">"},
                                          {LexemeKind::Op, "<="},
                                          {LexemeKind::Op, ">="},
                                          {LexemeKind::Op, "=="},
                                      });

  test_tokens("__abc_1", {{LexemeKind::Name, "__abc_1"}});

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

  test_vm("forward(10 + 20)", [](VM* vm) { ASSERT(eqf(vm->pos.y, -30.0), "y is -30.0"); });

  test_vm("forward(20 - 5)", [](VM* vm) { ASSERT(eqf(vm->pos.y, -15.0), "y is -15.0"); });

  test_vm("fn walk(x) { f(x) } walk(10)", [](VM* vm) { ASSERT(eqf(vm->pos.y, -10.0), "y is -10.0"); });

  test_vm("if (1.5 < 3.0) { f(10) } else { f(20) }", [](VM* vm) { ASSERT(eqf(vm->pos.y, -10.0), "y is -10.0"); });

  test_vm("if (1.5 > 3.0) { f(10) } else { f(20) }", [](VM* vm) { ASSERT(eqf(vm->pos.y, -20.0), "y is -20.0"); });

  test_vm("a = 123 f(a)", [](VM* vm) { ASSERT(eqf(vm->pos.y, -123.0), "y is -123.0"); });

  test_vm("a = rand(3, 4) f(a)", [](VM* vm) {
    ASSERT(vm->pos.y <= -3.0, "y is less than -3");
    ASSERT(vm->pos.y >= -4.0, "y is less than -4");
  });

  test_vm("f(10 * 10 + 10)", [](VM* vm) { ASSERT(eqf(vm->pos.y, -110.0), "y is -110.0"); });
  test_vm("f(10 + 10 * 10)", [](VM* vm) { ASSERT(eqf(vm->pos.y, -110.0), "y is -110.0"); });
  test_vm("f(5 + 10 * 10 + 10 - 5)", [](VM* vm) { ASSERT(eqf(vm->pos.y, -110.0), "y is -110.0"); });

  // Error scenarios:
  test_vm_raise("forward");
  test_vm_raise("forward()");
  test_vm_raise("forward(\"fsd\")");
  test_vm_raise("forward(1, 2)");
  test_vm_raise("forward(1 < 2)");
  test_vm_raise("forward(2 + \"few\")");

  // Value object testing.
  test_value();

  if (failCount == 0) {
    PASS("all");
  } else {
    FAIL("%d failed", failCount);
  }
}
