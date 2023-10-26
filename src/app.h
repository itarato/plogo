#pragma once

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <thread>
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
#define FLOATVARLIMIT 32

const vector<string> builtInFunctions{
    "forward [f]", "backward [b]", "left [l]",  "right [r]",     "up [u]",
    "down [d]",    "pos [p]",      "angle [a]", "thickness [t]", "rand",
    "clear [c]",   "intvar",       "floatvar",  "getx",          "gety",
    "getangle",    "push",         "pop",       "line",          "midx",
    "midy",        "debug"};

struct App {
  TextInput textInput{};
  VM vm{};

  int vstartx{0};
  int vstarty{0};
  int vstartangle{0};

  bool needScriptReload{false};

  char *sourceFileName{nullptr};
  chrono::time_point<chrono::file_clock> sourceFileUpdateTime;

  RenderTexture2D canvas;

  condition_variable condvarNeedParse;
  mutex mutexSourceCode;
  vector<string> sourceCodes{};
  atomic_bool needExit{false};

  App() {}
  App(const App &) = delete;
  App(App &&) = delete;

  void init() {
    int win_flags = FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT;
    SetConfigFlags(win_flags);

    InitWindow(config.win_w, config.win_h, "P-Logo V(0)");
    SetTargetFPS(120);

    rlImGuiSetup(true);

    textInput.init();
    canvas = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

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

    lock_guard<mutex> lock(mutexSourceCode);

    vm.reset();
    canvas = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    vm.pos.x = vstartx;
    vm.pos.y = vstarty;
    vm.angle = vstartangle;

    std::string fileContent;
    std::getline(std::ifstream(sourceFileName), fileContent, '\0');

    sourceCodes.push_back(fileContent);
    condvarNeedParse.notify_one();

    needScriptReload = false;
  }

  void sourceCodeInterpreterThread() {
    while (!needExit.load()) {
      INFO("THREAD LOOP");

      unique_lock<mutex> lock(mutexSourceCode);
      condvarNeedParse.wait(
          lock, [this] { return !sourceCodes.empty() || needExit.load(); });

      INFO("THREAD WAKE ");

      while (!sourceCodes.empty()) {
        auto fileContent = sourceCodes.back();
        sourceCodes.pop_back();
        try {
          runLogo(fileContent, &vm);
        } catch (runtime_error &e) {
          WARN("Compile error: %s", e.what());
        }
      }
    }
  }

  void reset() {
    vm.reset();
    vstartx = GetScreenWidth() >> 1;
    vstarty = GetScreenHeight() >> 1;
    vstartangle = 0;
  }

  void run() {
    thread parserThread([this] { sourceCodeInterpreterThread(); });

    while (!WindowShouldClose()) {
      update();

      BeginDrawing();

      ClearBackground(RAYWHITE);

      draw();
      drawPanel();

      EndDrawing();
    }

    needExit = true;

    INFO("EVENT LOOP ENDED %b", WindowShouldClose());

    condvarNeedParse.notify_one();
    parserThread.join();

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
        thread t([&]() { runLogo(command.value(), &vm); });
        t.join();
      } catch (runtime_error &e) {
        WARN("Compile error: %s", e.what());
      }
    }

    if (!vm.history.empty()) {
      BeginTextureMode(canvas);
      for (auto &line : vm.history) {
        DrawLineEx(line.from, line.to, line.thickness, line.color);
        // DrawLine(line.from.x, line.from.y, line.to.x, line.to.y, BLACK);
      }
      EndTextureMode();

      vm.history.clear();
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

        bool changed =
            ImGui::SliderInt(k.c_str(), intVarBackend + i, v.min, v.max);

        if (changed) {
          didChange = true;
          vm.frames.front().variables[k].floatVal = (float)intVarBackend[i];
        }

        i++;
      }

      int j = 0;
      for (auto &[k, v] : vm.floatVars) {
        floatVarBackend[j] = vm.frames.front().variables[k].floatVal;

        bool changed =
            ImGui::SliderFloat(k.c_str(), floatVarBackend + j, v.min, v.max);

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

      needScriptReload = didChange || vstartx != prevVstartx ||
                         vstarty != prevVstarty ||
                         vstartangle != prevVstartangle;
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

    DrawTexture(canvas.texture, 0, 0, WHITE);
    // DrawTextureRec(canvas.texture,
    //                (Rectangle){0, 0, (float)canvas.texture.width,
    //                            (float)-canvas.texture.height},
    //                (Vector2){0, 0}, WHITE);
  }
};
