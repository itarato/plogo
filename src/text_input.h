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
  vector<string> commandHistory{};
  int commandHistoryPtr{0};

  Font font;

  TextInput() {}

  TextInput(TextInput const &other) = delete;
  TextInput(TextInput &&other) = delete;
  ~TextInput() {}

  void init() {
    font = LoadFontEx("resources/fonts/JetBrainsMono-Regular.ttf", TEXT_SIZE,
                      nullptr, 255);
  }

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

      command.insert(cursor, 1, newChar);
      cursor++;
    }

    if (keyCode == KEY_BACKSPACE && !command.empty()) {
      command.erase(cursor - 1, 1);
      cursor--;
    }

    if (keyCode == KEY_ENTER) {
      string out{command};
      commandHistory.push_back(command);
      command.clear();
      cursor = 0;
      commandHistoryPtr = commandHistory.size();

      return out;
    }

    if (keyCode == KEY_LEFT) cursor--;
    if (keyCode == KEY_RIGHT) cursor++;
    cursor = toRange(cursor, 0, (int)command.size());

    if (keyCode == KEY_UP && commandHistoryPtr > 0) {
      commandHistoryPtr--;
      command = commandHistory[commandHistoryPtr];
      cursor = command.size();
    }
    if (keyCode == KEY_DOWN) {
      if ((commandHistoryPtr + 1) < (int)commandHistory.size()) {
        commandHistoryPtr++;
        command = commandHistory[commandHistoryPtr];
        cursor = command.size();
      } else if ((commandHistoryPtr + 1) == (int)commandHistory.size()) {
        commandHistoryPtr++;
        command.clear();
        cursor = command.size();
      }
    }

    return nullopt;
  }

  void draw() const {
    DrawRectangle(0, GetScreenHeight() - TEXT_SIZE - TEXT_MARGIN * 2,
                  GetScreenWidth(), GetScreenHeight(), LIGHTGRAY);

    const int shevronPadding{18};

    DrawTextEx(font, ">",
               Vector2{(float)TEXT_MARGIN,
                       (float)(GetScreenHeight() - TEXT_MARGIN - TEXT_SIZE)},
               (float)TEXT_SIZE, 0.0f, BLACK);

    DrawTextEx(font, command.c_str(),
               Vector2{(float)(TEXT_MARGIN + shevronPadding),
                       (float)(GetScreenHeight() - TEXT_MARGIN - TEXT_SIZE)},
               (float)TEXT_SIZE, 0.0f, BLACK);

    string preCursor = command.substr(0, min((int)command.size(), cursor));
    auto preCursorLen =
        MeasureTextEx(font, preCursor.c_str(), (float)TEXT_SIZE, 0.0f);
    DrawLine(preCursorLen.x + TEXT_MARGIN + shevronPadding,
             GetScreenHeight() - TEXT_MARGIN - TEXT_SIZE,
             preCursorLen.x + TEXT_MARGIN + shevronPadding,
             GetScreenHeight() - TEXT_MARGIN, BLACK);
  }
};
