#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "raylib.h"
#include "util.h"

using namespace std;

#define TEXT_MARGIN 4
#define TEXT_SIZE 10

struct TextInput {
  string command{};
  bool isActive = true;

  TextInput() {}

  TextInput(TextInput const &other) = delete;
  TextInput(TextInput &&other) = delete;
  ~TextInput() {}

  void update() {
    TraceLog(LOG_INFO, "Here");

    if (!isActive) return;

    // Text input.
    int keyCode = GetKeyPressed();
    if (keyCode >= 32 && keyCode <= 126) {
      command.push_back(char(keyCode));
    }

    if (keyCode == KEY_BACKSPACE && !command.empty()) {
      command.pop_back();
    }
  }

  void draw() const {
    DrawText(command.c_str(), TEXT_MARGIN,
             GetScreenHeight() - TEXT_MARGIN - TEXT_SIZE, TEXT_SIZE, BLACK);
  }
};
