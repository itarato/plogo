#include <iostream>
#include <utility>

#include "logo.h"
#include "vm.h"

#define FAIL(...) log("\x1b[91mFAIL\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)
#define PASS(...) log("\x1b[92mPASS\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...) log("\x1b[90mINFO\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)

#define ASSERT(exp, msg) (exp ? PASS(msg) : FAIL(msg))

using namespace std;

void log(const char* level, const char* fileName, int lineNo, const char* s,
         ...) {
  va_list args;
  va_start(args, s);

  printf("[%s][\x1b[93m%s\x1b[39m:\x1b[96m%d\x1b[0m] \x1b[94m", level, fileName,
         lineNo);
  vprintf(s, args);
  printf("\x1b[0m\n");

  va_end(args);
}

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

  PASS("all");
}
