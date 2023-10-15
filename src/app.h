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
  Texture2D turtleTexture;

  App() {}
  App(const App &) = delete;
  App(App &&) = delete;

  void init() {
    int win_flags = FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT;
    SetConfigFlags(win_flags);

    InitWindow(config.win_w, config.win_h, "P-Logo");
    SetTargetFPS(60);

    reset();

    turtleTexture = LoadTexture("./resource/image/turtle.png");
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

    for (auto &line : vm.history) {
      DrawLineV(line.from, line.to, BLACK);
    }

    DrawTexturePro(turtleTexture,
                   Rectangle{0.0f, 0.0f, float(turtleTexture.width),
                             float(turtleTexture.height)},
                   Rectangle{vm.pos.x, vm.pos.y, float(turtleTexture.width),
                             float(turtleTexture.height)},

                   Vector2{float(turtleTexture.width) / 2.0f,
                           float(turtleTexture.height) / 2.0f},
                   vm.angle, WHITE);
  }
};
