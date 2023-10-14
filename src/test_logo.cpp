#include "logo.h"
#include "vm.h"

int main() {
  cout << "Test parser\n";
  Lexer lexer{"forward(10)"};
  auto lexemes = lexer.parse();
  for (auto lexeme : lexemes) {
    cout << "Lexeme(" << lexemeKindToString(lexeme.kind) << ") = " << lexeme.v
         << endl;
  }

  Parser parser{lexemes};
  Ast::Program prg = parser.parse();
  VM vm{};

  prg.execute(&vm);
}
