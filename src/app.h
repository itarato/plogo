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
constexpr int SCRIPT_RELOAD_LIGHT = 1;
constexpr int SCRIPT_RELOAD_LIGHT_AND_STATE = 2;
constexpr int SCRIPT_RELOAD_FULL = 3;

constexpr int INTVARLIMIT = 64;
constexpr int FLOATVARLIMIT = 64;

// We need to render the logo drawing high scale as textures rasterize lines without smoothing. Downscaling gives
// us a little bit of smooothing. 2 seems to be the sweet spot with trilinear texture filter.
constexpr float DRAW_TEXTURE_SCALE = 2.f;

const vector<string> builtInFunctions{
    "[f]orward(NUM)",
    "[b]ackward(NUM)",
    "[l]eft(NUM)",
    "[r]ight(NUM)",
    "[u]p()",
    "[d]own()",
    "[p]os(NUM, NUM)",
    "[a]ngle(NUM)",
    "[t]hickness(NUM)",
    "[c]lear()",
    "rand(NUM, NUM) -> NUM",
    "intvar(STR, NUM, NUM, NUM)",
    "floatvar(STR, NUM, NUM, NUM)",
    "getx() -> NUM",
    "gety() -> NUM",
    "getangle() -> NUM",
    "push(NUM, ...)",
    "pop() -> NUM",
    "line(NUM, NUM, NUM, NUM)",
    "winw() -> NUM",
    "winh() -> NUM",
    "midx() -> NUM",
    "midy() -> NUM",
    "debug(NUM, ...)",
};

struct App {
  TextInput textInput{};
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

  bool showSourceCode{true};

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

    textInput.init();

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

    needScriptReload = SCRIPT_RELOAD_FULL;
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

    vm.reset(needScriptReload >= SCRIPT_RELOAD_FULL, needScriptReload >= SCRIPT_RELOAD_LIGHT_AND_STATE);

    if (needScriptReload >= SCRIPT_RELOAD_LIGHT_AND_STATE) {
      vm.pos.x = vstartx;
      vm.pos.y = vstarty;
      vm.angle = vstartangle;
    }

    runLogo(sourceCode, &vm, &lastRenderTime);
    needDrawTextureRedraw = true;

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
      needScriptReload = SCRIPT_RELOAD_LIGHT_AND_STATE;
    }

    checkSourceForUpdates();

    if (needScriptReload > SCRIPT_RELOAD_NO) scriptReload();

    if (!showSourceCode) {
      auto command = textInput.update();
      if (command.has_value()) {
        runLogo(command.value().c_str(), &vm, &lastRenderTime);
        needDrawTextureRedraw = true;
      }
    }
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
    drawToolbarLog();
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
      needScriptReload = SCRIPT_RELOAD_LIGHT_AND_STATE;
    }
  }

  void drawSourceCode() {
    showSourceCode = ImGui::CollapsingHeader("Source code", ImGuiTreeNodeFlags_DefaultOpen);

    if (showSourceCode) {
      ImGui::InputTextMultiline("source_code", sourceCode, IM_ARRAYSIZE(sourceCode),
                                ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 32), ImGuiInputTextFlags_AllowTabInput);

      if (ImGui::Button("Clear and run")) needScriptReload = SCRIPT_RELOAD_FULL;
      ImGui::SameLine();
      if (ImGui::Button("Run")) needScriptReload = SCRIPT_RELOAD_LIGHT;
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

  void drawToolbarLog() {
    if (ImGui::CollapsingHeader("Logs")) {
      ImGui::Text(appLog.aggregated.c_str());
    }
  }

  void drawToolbarHelp() {
    if (ImGui::CollapsingHeader("Reference")) {
      ImGui::TextColored({1.0, 1.0, 0.6, 1.0}, "Custom functions:");
      for (auto &[k, v] : vm.functions) {
        string signature{k};
        signature += "(";

        for (int i = 0; i < v->argNames.size(); i++) {
          signature += v->argNames[i];
          if (i < v->argNames.size() - 1) signature += ",";
        }

        signature += ")";
        ImGui::BulletText("%s", signature.c_str());
      }

      ImGui::Separator();

      ImGui::TextColored({1.0, 1.0, 0.6, 1.0}, "Built in functions:");
      for (auto e : builtInFunctions) {
        ImGui::BulletText("%s", e.c_str());
      }
    }
  }

  void draw() const {
    DrawTextureEx(drawTexture.texture, Vector2Zero(), 0.f, 1.f / DRAW_TEXTURE_SCALE, WHITE);

    if (!showSourceCode) textInput.draw();

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
