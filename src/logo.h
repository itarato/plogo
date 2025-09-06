#pragma once

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

void runLogo(const char *code, VM *vm, float *renderTime) {
  TraceLog(LOG_INFO, "Compile start");
  double t_start = GetTime();

  try {
    Lexer lexer{code};
    Parser parser{lexer.parse()};
    Ast::Program prg = parser.parse();
    prg.execute(vm);
  } catch (runtime_error &e) {
    WARN("Compile error: %s", e.what());
    appLog.append(TextFormat("[ERROR] compile error: %s", e.what()));
  }

  *renderTime = GetTime() - t_start;

  TraceLog(LOG_INFO, "Compile end. Latency: %.2f ms", *renderTime * 1000.0);
}
