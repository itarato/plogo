#pragma once

#include <fstream>
#include <optional>
#include <vector>

#include "ast.h"
#include "config.h"
#include "imgui.h"
#include "logo.h"
#include "parser.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "text_input.h"
#include "vm.h"

using namespace std;

#define INTVARLIMIT 32

struct App {
  TextInput textInput{};
  VM vm{};

  int vstartx{0};
  int vstarty{0};
  int vstartangle{0};

  bool needScriptReload{false};

  char *sourceFileName{nullptr};

  App() {}
  App(const App &) = delete;
  App(App &&) = delete;

  void init() {
    int win_flags = FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT;
    SetConfigFlags(win_flags);

    InitWindow(config.win_w, config.win_h, "P-Logo V(0)");
    SetTargetFPS(24);

    rlImGuiSetup(true);

    reset();
  }

  void setSourceFile(char *fileName) {
    sourceFileName = fileName;
    scriptReload();
  }

  void scriptReload() {
    if (sourceFileName == nullptr) return;

    vm.reset();

    vm.pos.x = vstartx;
    vm.pos.y = vstarty;
    vm.angle = vstartangle;

    std::string fileContent;
    std::getline(std::ifstream(sourceFileName), fileContent, '\0');

    try {
      runLogo(fileContent, &vm);
    } catch (runtime_error &e) {
      WARN("Compile error: %s", e.what());
    }

    needScriptReload = false;
  }

  void reset() {
    vm.reset();
    vstartx = GetScreenWidth() >> 1;
    vstarty = GetScreenHeight() >> 1;
    vstartangle = 0;
  }

  void run() {
    while (!WindowShouldClose()) {
      update();

      BeginDrawing();

      ClearBackground(RAYWHITE);

      draw();

      drawPanel();

      EndDrawing();
    }

    rlImGuiShutdown();

    CloseWindow();
  }

  void update() {
    if (IsMouseButtonPressed(1)) {
      vstartx = GetMousePosition().x;
      vstarty = GetMousePosition().y;
      vm.setPos(GetMousePosition().x, GetMousePosition().y);
      needScriptReload = true;
    }

    if (needScriptReload) scriptReload();

    auto command = textInput.update();
    if (command.has_value()) {
      try {
        runLogo(command.value(), &vm);
      } catch (runtime_error &e) {
        WARN("Compile error: %s", e.what());
      }
    }
  }

  void drawPanel() {
    rlImGuiBegin();

    bool didChange{false};
    int prevVstartx{vstartx};
    int prevVstarty{vstarty};
    int prevVstartangle{vstartangle};

    ImGui::Begin("Variables");

    int intVarBackend[INTVARLIMIT];
    assert(vm.intVars.size() < INTVARLIMIT);

    int i = 0;
    for (auto &[k, v] : vm.intVars) {
      intVarBackend[i] = (int)vm.frames.front().variables[k].floatVal;

      bool changed =
          ImGui::SliderInt(k.c_str(), intVarBackend + i, v.min, v.max);

      if (changed) {
        didChange = true;
        vm.frames.front().variables[k].floatVal = (float)intVarBackend[i];
      }

      i++;
    }

    ImGui::SliderInt("Start x", &vstartx, 0, GetScreenWidth());
    ImGui::SliderInt("Start y", &vstarty, 0, GetScreenHeight());
    ImGui::SliderInt("Start angle", &vstartangle, 0, 360);

    ImGui::End();

    rlImGuiEnd();

    needScriptReload = didChange || vstartx != prevVstartx ||
                       vstarty != prevVstarty || vstartangle != prevVstartangle;
  }

  void draw() const {
    DrawFPS(GetScreenWidth() - 100, 4);
    DrawText(TextFormat("Line count: %d", vm.history.size()),
             GetScreenWidth() - 100, 28, 10, BLACK);

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
      DrawLineEx(line.from, line.to, line.thickness, line.color);
    }
  }
};
