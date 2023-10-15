#pragma once

#include <optional>
#include <vector>

#include "config.h"
#include "logo.h"
#include "raylib.h"
#include "text_input.h"
#include "vm.h"

using namespace std;

struct App {
  TextInput textInput{};
  VM vm{};
  optional<string> command{nullopt};

  App() {}
  App(const App &) = delete;
  App(App &&) = delete;

  void init() {
    int win_flags = FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT;
    SetConfigFlags(win_flags);

    InitWindow(config.win_w, config.win_h, "P-Logo");
    SetTargetFPS(60);

    reset();
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

  void update() { command = textInput.update(); }

  void draw() {
    DrawFPS(GetScreenWidth() - 100, 4);
    textInput.draw();

    if (command.has_value()) {
      Lexer lexer{command.value()};
      Parser parser{lexer.parse()};
      Ast::Program prg = parser.parse();
      prg.execute(&vm);
    }
  }
};
