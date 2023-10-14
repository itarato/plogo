#pragma once

#include <algorithm>
#include <optional>
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

  optional<string> update() {
    if (!isActive) return nullopt;

    // Text input.
    int keyCode = GetKeyPressed();
    if (keyCode >= 32 && keyCode <= 126) {
      command.push_back(char(keyCode));
    }

    if (keyCode == KEY_BACKSPACE && !command.empty()) {
      command.pop_back();
    }

    if (keyCode == KEY_ENTER) {
      return command;
    } else {
      return nullopt;
    }
  }

  void draw() const {
    DrawText(command.c_str(), TEXT_MARGIN,
             GetScreenHeight() - TEXT_MARGIN - TEXT_SIZE, TEXT_SIZE, BLACK);
  }
};
