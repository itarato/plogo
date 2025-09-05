#pragma once

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

void runLogo(string code, VM *vm) {
  TraceLog(LOG_INFO, "Compile start");
  double t_start = GetTime();

  Lexer lexer{code};
  Parser parser{lexer.parse()};
  Ast::Program prg = parser.parse();
  prg.execute(vm);

  TraceLog(LOG_INFO, "Compile start. Latency: %.2f ms", (GetTime() - t_start) * 1000.0);
}
