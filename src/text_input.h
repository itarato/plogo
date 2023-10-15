#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "raylib.h"
#include "util.h"

using namespace std;

#define TEXT_MARGIN 4
#define TEXT_SIZE 20

struct TextInput {
  string command{};
  bool isActive = true;
  bool shiftOn = false;

  TextInput() {}

  TextInput(TextInput const &other) = delete;
  TextInput(TextInput &&other) = delete;
  ~TextInput() {}

  optional<string> update() {
    if (!isActive) return nullopt;

    // Text input.
    int keyCode = GetKeyPressed();
    if (keyCode >= 32 && keyCode <= 126) {
      char newChar;
      if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        switch (keyCode) {
          case KEY_NINE:
            newChar = '(';
            break;
          case KEY_ZERO:
            newChar = ')';
            break;
          case KEY_LEFT_BRACKET:
            newChar = '{';
            break;
          case KEY_RIGHT_BRACKET:
            newChar = '}';
            break;
          default:
            newChar = char(keyCode);
            break;
        }
      } else {
        newChar = tolower(char(keyCode));
      }

      command.push_back(newChar);
    }

    if (keyCode == KEY_BACKSPACE && !command.empty()) {
      command.pop_back();
    }

    if (keyCode == KEY_ENTER) {
      string out{command};
      command.clear();
      return out;
    } else {
      return nullopt;
    }
  }

  void draw() const {
    DrawText(command.c_str(), TEXT_MARGIN,
             GetScreenHeight() - TEXT_MARGIN - TEXT_SIZE, TEXT_SIZE, BLACK);
  }
};
