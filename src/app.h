#pragma once

#include <fstream>
#include <optional>
#include <vector>

#include "config.h"
#include "logo.h"
#include "raylib.h"
#include "raymath.h"
#include "text_input.h"
#include "vm.h"

using namespace std;

struct App {
  TextInput textInput{};
  VM vm{};

  char *sourceFileName{nullptr};

  App() {}
  App(const App &) = delete;
  App(App &&) = delete;

  void init() {
    int win_flags = FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT;
    SetConfigFlags(win_flags);

    InitWindow(config.win_w, config.win_h, "P-Logo");
    SetTargetFPS(24);

    reset();
  }

  void setSourceFile(char *fileName) {
    sourceFileName = fileName;
    vm.reset();

    std::string fileContent;
    std::getline(std::ifstream(fileName), fileContent, '\0');

    Lexer lexer{fileContent};
    Parser parser{lexer.parse()};
    Ast::Program prg = parser.parse();
    prg.execute(&vm);
  }

  void reset() { vm.reset(); }

  void run() {
    while (!WindowShouldClose()) {
      update();

      BeginDrawing();

      ClearBackground(RAYWHITE);

      draw();

      EndDrawing();
    }
  }

  void update() {
    auto command = textInput.update();

    if (command.has_value()) {
      Lexer lexer{command.value()};
      Parser parser{lexer.parse()};
      Ast::Program prg = parser.parse();
      prg.execute(&vm);
    }
  }

  void draw() const {
    DrawFPS(GetScreenWidth() - 100, 4);
    textInput.draw();

    // Draw turtle (triangle).
    Vector2 p1 =
        Vector2Add(Vector2Rotate(Vector2{0.0f, -12.0f}, vm.rad()), vm.pos);
    Vector2 p2 =
        Vector2Add(Vector2Rotate(Vector2{-6.0f, 8.0f}, vm.rad()), vm.pos);
    Vector2 p3 =
        Vector2Add(Vector2Rotate(Vector2{6.0f, 8.0f}, vm.rad()), vm.pos);
    DrawTriangle(p1, p2, p3, GREEN);

    for (auto &line : vm.history) {
      DrawLineV(line.from, line.to, BLACK);
    }
  }
};
