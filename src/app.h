#pragma once

#include <chrono>
#include <cstring>
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

constexpr int SCRIPT_RELOAD_NO = 0;
constexpr int SCRIPT_RELOAD_SOFT = 1;
constexpr int SCRIPT_RELOAD_HARD = 2;
constexpr int INTVARLIMIT = 32;
constexpr int FLOATVARLIMIT = 32;
constexpr float DRAW_TEXTURE_SCALE = 2.f;

const vector<string> builtInFunctions{
    "forward [f]",   "backward [b]", "left [l]",  "right [r]", "up [u]",   "down [d]", "pos [p]", "angle [a]",
    "thickness [t]", "rand",         "clear [c]", "intvar",    "floatvar", "getx",     "gety",    "getangle",
    "push",          "pop",          "line",      "midx",      "midy",     "debug"};

struct App {
  // TextInput textInput{};
  VM vm{};
  RenderTexture2D drawTexture;

  int vstartx{0};
  int vstarty{0};
  int vstartangle{0};

  int needScriptReload{SCRIPT_RELOAD_NO};
  bool needDrawTextureRedraw{false};

  char *sourceFileName{nullptr};
  chrono::time_point<chrono::file_clock> sourceFileUpdateTime{};

  int intVarBackend[INTVARLIMIT];
  float floatVarBackend[FLOATVARLIMIT];

  int winWidth;
  int winHeight;

  float lastRenderTime{};

  char sourceCode[2048]{};

  App() = default;

  App(const App &) = delete;

  App(App &&) = delete;

  void destruct_assets() {
    UnloadRenderTexture(drawTexture);
  }

  void init() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(config.win_w, config.win_h, "P-Logo V(0)");
    SetTargetFPS(24);

    init_render_texture();

    rlImGuiSetup(true);

    // textInput.init();

    winWidth = GetScreenWidth();
    winHeight = GetScreenHeight();

    vm.reset();

    vstartx = GetScreenWidth() >> 1;
    vstarty = GetScreenHeight() >> 1;
    vstartangle = 0;
  }

  void init_render_texture() {
    drawTexture = LoadRenderTexture(GetScreenWidth() * DRAW_TEXTURE_SCALE, GetScreenHeight() * DRAW_TEXTURE_SCALE);
    SetTextureFilter(drawTexture.texture, TEXTURE_FILTER_TRILINEAR);
  }

  void loadSourceFile(char *_sourceFileName) {
    sourceFileName = _sourceFileName;
    if (sourceFileName == nullptr) return;

    INFO("Loading script: %s", sourceFileName);

    sourceFileUpdateTime = getSourceFileUpdateTime();

    std::string fileContent;
    std::getline(std::ifstream(sourceFileName), fileContent, '\0');

    bzero(sourceCode, sizeof(sourceCode));
    TraceLog(LOG_INFO, "Zero %d bytes", sizeof(sourceCode));
    if (sizeof(sourceCode) > fileContent.size()) {
      strncpy(sourceCode, fileContent.c_str(), fileContent.size());
    } else {
      TraceLog(LOG_WARNING, "Source code exceeds input buffer size");
    }

    needScriptReload = SCRIPT_RELOAD_HARD;
    scriptReload();
  }

  void scriptReload() {
    INFO("Reloading script");

    int i = 0;
    for (auto &[k, v] : vm.intVars) {
      vm.frames.front().variables[k].floatVal = (float)intVarBackend[i];
      i++;
    }

    i = 0;
    for (auto &[k, v] : vm.floatVars) {
      vm.frames.front().variables[k].floatVal = floatVarBackend[i];
      i++;
    }

    vm.reset(needScriptReload >= SCRIPT_RELOAD_HARD);
    vm.pos.x = vstartx;
    vm.pos.y = vstarty;
    vm.angle = vstartangle;

    try {
      runLogo(sourceCode, &vm, &lastRenderTime);
      needDrawTextureRedraw = true;
    } catch (runtime_error &e) {
      WARN("Compile error: %s", e.what());
    }

    i = 0;
    for (auto &[k, v] : vm.intVars) {
      intVarBackend[i] = (int)vm.frames.front().variables[k].floatVal;
      i++;
    }

    i = 0;
    for (auto &[k, v] : vm.floatVars) {
      floatVarBackend[i] = vm.frames.front().variables[k].floatVal;
      i++;
    }

    needScriptReload = SCRIPT_RELOAD_NO;
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
    if (winWidth != GetScreenWidth() || winHeight != GetScreenHeight()) {
      winWidth = GetScreenWidth();
      winHeight = GetScreenHeight();

      UnloadRenderTexture(drawTexture);
      init_render_texture();
    }

    if (IsMouseButtonPressed(1)) {
      vstartx = GetMousePosition().x;
      vstarty = GetMousePosition().y;
      needScriptReload = SCRIPT_RELOAD_SOFT;
    }

    checkSourceForUpdates();

    if (needScriptReload > SCRIPT_RELOAD_NO) scriptReload();

    // auto command = textInput.update();
    // if (command.has_value()) {
    //   try {
    //     runLogo(command.value(), &vm, &lastRenderTime);
    //     needDrawTextureRedraw = true;
    //   } catch (runtime_error &e) {
    //     WARN("Compile error: %s", e.what());
    //   }
    // }
  }

  void checkSourceForUpdates() {
    if (sourceFileName == nullptr) return;

    auto currentTime = getSourceFileUpdateTime();
    if (currentTime != sourceFileUpdateTime) {
      loadSourceFile(sourceFileName);
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
    drawSourceCode();
    drawToolbarDebug();
    drawToolbarHelp();

    ImGui::End();
    rlImGuiEnd();
  }

  void drawToolbarVariables() {
    bool didChange{false};
    int prevVstartx{vstartx};
    int prevVstarty{vstarty};
    int prevVstartangle{vstartangle};

    assert(vm.intVars.size() < INTVARLIMIT);
    assert(vm.floatVars.size() < FLOATVARLIMIT);

    int i = 0;
    for (auto &[k, v] : vm.intVars) {
      bool changed = ImGui::SliderInt(k.c_str(), intVarBackend + i, v.min, v.max);

      if (changed) didChange = true;

      i++;
    }

    int j = 0;
    for (auto &[k, v] : vm.floatVars) {
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

    if (needScriptReload == SCRIPT_RELOAD_NO &&
        (didChange || vstartx != prevVstartx || vstarty != prevVstarty || vstartangle != prevVstartangle)) {
      needScriptReload = SCRIPT_RELOAD_SOFT;
    }
  }

  void drawSourceCode() {
    if (ImGui::CollapsingHeader("Source code", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::InputTextMultiline("source_code", sourceCode, IM_ARRAYSIZE(sourceCode),
                                ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 32), ImGuiInputTextFlags_AllowTabInput);

      if (ImGui::Button("Compile")) needScriptReload = SCRIPT_RELOAD_HARD;
    }
  }

  void drawToolbarDebug() {
    if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("FPS: %d", GetFPS());
      ImGui::Text("Edge count: %'lu", vm.history.size());
      ImGui::Text("Render time: %.2f ms", lastRenderTime * 1000.f);

      ImGui::Separator();

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

    // textInput.draw();

    // Draw turtle (triangle).
    Vector2 p1 = Vector2Add(Vector2Rotate(Vector2{0.0f, -12.0f}, vm.rad()), vm.pos);
    Vector2 p2 = Vector2Add(Vector2Rotate(Vector2{-6.0f, 8.0f}, vm.rad()), vm.pos);
    Vector2 p3 = Vector2Add(Vector2Rotate(Vector2{6.0f, 8.0f}, vm.rad()), vm.pos);
    DrawTriangle(p1, p2, p3, GREEN);
  }

  void draw_draw_texture() {
    if (!needDrawTextureRedraw) return;

    Vector2 start{};
    Vector2 end{};

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
};
