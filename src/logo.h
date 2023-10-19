#pragma once

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

void runLogo(string code, VM* vm) {
  Lexer lexer{code};
  Parser parser{lexer.parse()};
  Ast::Program prg = parser.parse();
  prg.execute(vm);
}
