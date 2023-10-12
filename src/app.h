#pragma once

#include <vector>

#include "config.h"
#include "raylib.h"
#include "text_input.h"

using namespace std;

struct App {
  TextInput textInput{};

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

  void reset() {}

  void run() {
    while (!WindowShouldClose()) {
      update();

      BeginDrawing();

      ClearBackground(RAYWHITE);

      draw();

      EndDrawing();
    }
  }

  void update() { textInput.update(); }

  void draw() const {
    DrawFPS(GetScreenWidth() - 100, 4);
    textInput.draw();
  }
};
