#pragma once

#include <chrono>
#include <filesystem>
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
#include "util.h"
#include "vm.h"

using namespace std;

constexpr int INTVARLIMIT = 32;
constexpr int FLOATVARLIMIT = 32;
constexpr float DRAW_TEXTURE_SCALE = 2.f;

const vector<string> builtInFunctions{
    "forward [f]",   "backward [b]", "left [l]",  "right [r]", "up [u]",   "down [d]", "pos [p]", "angle [a]",
    "thickness [t]", "rand",         "clear [c]", "intvar",    "floatvar", "getx",     "gety",    "getangle",
    "push",          "pop",          "line",      "midx",      "midy",     "debug"};

struct App {
  TextInput textInput{};
  VM vm{};
  RenderTexture2D drawTexture;

  int vstartx{0};
  int vstarty{0};
  int vstartangle{0};

  bool needScriptReload{false};
  bool needDrawTextureRedraw{false};

  char *sourceFileName{nullptr};
  chrono::time_point<chrono::file_clock> sourceFileUpdateTime;

  App() = default;

  App(const App &) = delete;

  App(App &&) = delete;

  void destruct_assets() {
    UnloadRenderTexture(drawTexture);
  }

  void init() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(config.win_w, config.win_h, "P-Logo V(0)");
    SetTargetFPS(24);

    drawTexture = LoadRenderTexture(GetScreenWidth() * DRAW_TEXTURE_SCALE, GetScreenHeight() * DRAW_TEXTURE_SCALE);
    SetTextureFilter(drawTexture.texture, TEXTURE_FILTER_TRILINEAR);

    rlImGuiSetup(true);

    textInput.init();

    reset();
  }

  void setSourceFile(char *fileName) {
    sourceFileName = fileName;
    scriptReload();
  }

  void scriptReload() {
    if (sourceFileName == nullptr) return;

    INFO("Reloading script: %s", sourceFileName);

    sourceFileUpdateTime = getSourceFileUpdateTime();

    vm.reset();

    vm.pos.x = vstartx;
    vm.pos.y = vstarty;
    vm.angle = vstartangle;

    std::string fileContent;
    std::getline(std::ifstream(sourceFileName), fileContent, '\0');

    try {
      runLogo(fileContent, &vm);
      needDrawTextureRedraw = true;
    } catch (runtime_error &e) {
      WARN("Compile error: %s", e.what());
    }
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

      draw_draw_texture();

      BeginDrawing();

      ClearBackground(RAYWHITE);

      draw();
      drawPanel();

      EndDrawing();
    }

    destruct_assets();

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

    checkSourceForUpdates();

    if (needScriptReload) scriptReload();

    auto command = textInput.update();
    if (command.has_value()) {
      try {
        runLogo(command.value(), &vm);
        needDrawTextureRedraw = true;
      } catch (runtime_error &e) {
        WARN("Compile error: %s", e.what());
      }
    }
  }

  void checkSourceForUpdates() {
    if (sourceFileName == nullptr) return;

    auto currentTime = getSourceFileUpdateTime();
    if (currentTime != sourceFileUpdateTime) {
      vm.hardReset();
      scriptReload();
    }
  }

  chrono::time_point<chrono::file_clock> getSourceFileUpdateTime() {
    filesystem::path p{sourceFileName};
    return filesystem::last_write_time(p);
  }

  void drawPanel() {
    rlImGuiBegin();
    ImGui::Begin("Toolbar");

    drawToolbarVariables();
    drawToolbarHelp();
    drawToolbarDebug();

    ImGui::End();
    rlImGuiEnd();
  }

  void drawToolbarVariables() {
    if (ImGui::CollapsingHeader("Variables", ImGuiTreeNodeFlags_DefaultOpen)) {
      bool didChange{false};
      int prevVstartx{vstartx};
      int prevVstarty{vstarty};
      int prevVstartangle{vstartangle};

      int intVarBackend[INTVARLIMIT];
      assert(vm.intVars.size() < INTVARLIMIT);
      float floatVarBackend[FLOATVARLIMIT];
      assert(vm.floatVars.size() < FLOATVARLIMIT);

      int i = 0;
      for (auto &[k, v] : vm.intVars) {
        intVarBackend[i] = (int)vm.frames.front().variables[k].floatVal;

        bool changed = ImGui::SliderInt(k.c_str(), intVarBackend + i, v.min, v.max);

        if (changed) {
          didChange = true;
          vm.frames.front().variables[k].floatVal = (float)intVarBackend[i];
        }

        i++;
      }

      int j = 0;
      for (auto &[k, v] : vm.floatVars) {
        floatVarBackend[j] = vm.frames.front().variables[k].floatVal;

        bool changed = ImGui::SliderFloat(k.c_str(), floatVarBackend + j, v.min, v.max);

        if (changed) {
          didChange = true;
          vm.frames.front().variables[k].floatVal = floatVarBackend[j];
        }

        j++;
      }

      ImGui::Separator();

      ImGui::SliderInt("Start x", &vstartx, 0, GetScreenWidth());
      ImGui::SliderInt("Start y", &vstarty, 0, GetScreenHeight());
      ImGui::SliderInt("Start angle", &vstartangle, 0, 360);

      needScriptReload =
          didChange || vstartx != prevVstartx || vstarty != prevVstarty || vstartangle != prevVstartangle;
    }
  }

  void drawToolbarDebug() {
    if (ImGui::CollapsingHeader("Debug")) {
      ImGui::TextColored({1.0, 1.0, 0.6, 1.0}, "Root variables:");
      ImGui::BulletText("Position -> x = %.2f y = %.2f", vm.pos.x, vm.pos.y);
      ImGui::BulletText("Angle -> %.2f", vm.angle);
      ImGui::BulletText("Thickness -> %.2f", vm.thickness);

      ImGui::Separator();

      ImGui::TextColored({1.0, 1.0, 0.6, 1.0}, "Top frame variables:");
      for (auto &[k, v] : vm.frames.front().variables) {
        ImGui::BulletText("%s = %.2f", k.c_str(), v.floatVal);
      }
    }
  }

  void drawToolbarHelp() {
    if (ImGui::CollapsingHeader("Help")) {
      ImGui::TextColored({1.0, 1.0, 0.6, 1.0}, "Built in functions:");
      for (auto e : builtInFunctions) {
        ImGui::BulletText("%s", e.c_str());
      }

      ImGui::Separator();

      ImGui::TextColored({1.0, 1.0, 0.6, 1.0}, "Custom functions:");
      for (auto &[k, _v] : vm.functions) {
        ImGui::BulletText("%s", k.c_str());
      }
    }
  }

  void draw() const {
    DrawTextureEx(drawTexture.texture, Vector2Zero(), 0.f, 1.f / DRAW_TEXTURE_SCALE, WHITE);

    DrawFPS(GetScreenWidth() - 100, 4);
    DrawText(TextFormat("Line count: %d", vm.history.size()), GetScreenWidth() - 100, 28, 10, BLACK);

    textInput.draw();

    // Draw turtle (triangle).
    Vector2 p1 = Vector2Add(Vector2Rotate(Vector2{0.0f, -12.0f}, vm.rad()), vm.pos);
    Vector2 p2 = Vector2Add(Vector2Rotate(Vector2{-6.0f, 8.0f}, vm.rad()), vm.pos);
    Vector2 p3 = Vector2Add(Vector2Rotate(Vector2{6.0f, 8.0f}, vm.rad()), vm.pos);
    DrawTriangle(p1, p2, p3, GREEN);
  }

  void draw_draw_texture() {
    Vector2 start{};
    Vector2 end{};

    if (needDrawTextureRedraw) {
      BeginTextureMode(drawTexture);
      DrawRectangle(0, 0, GetScreenWidth() * DRAW_TEXTURE_SCALE, GetScreenHeight() * DRAW_TEXTURE_SCALE, WHITE);
      for (auto const &line : vm.history) {
        start.x = line.from.x * DRAW_TEXTURE_SCALE;
        start.y = (GetScreenHeight() - line.from.y) * DRAW_TEXTURE_SCALE;

        end.x = line.to.x * DRAW_TEXTURE_SCALE;
        end.y = (GetScreenHeight() - line.to.y) * DRAW_TEXTURE_SCALE;

        DrawLineEx(start, end, line.thickness * DRAW_TEXTURE_SCALE, line.color);
      }
      EndTextureMode();
      needDrawTextureRedraw = false;
    }
  }
};
