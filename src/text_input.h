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
  int cursor = 0;

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
          case KEY_APOSTROPHE:
            newChar = '\"';
            break;
          default:
            newChar = char(keyCode);
            break;
        }
      } else {
        newChar = tolower(char(keyCode));
      }

      command.push_back(newChar);
      cursor++;
    }

    if (keyCode == KEY_BACKSPACE && !command.empty()) {
      command.pop_back();
      cursor--;
    }

    if (keyCode == KEY_ENTER) {
      string out{command};
      command.clear();
      cursor = 0;

      return out;
    } else {
      return nullopt;
    }
  }

  void draw() const {
    DrawText(command.c_str(), TEXT_MARGIN,
             GetScreenHeight() - TEXT_MARGIN - TEXT_SIZE, TEXT_SIZE, BLACK);

    string preCursor = command.substr(0, min((int)command.size(), cursor));
    auto preCursorLen = MeasureText(preCursor.c_str(), TEXT_SIZE);
    DrawLine(
        preCursorLen + TEXT_MARGIN, GetScreenHeight() - TEXT_MARGIN - TEXT_SIZE,
        preCursorLen + TEXT_MARGIN, GetScreenHeight() - TEXT_MARGIN, BLACK);
  }
};
