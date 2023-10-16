#pragma once

#include <cmath>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

#include "logo.h"
#include "raylib.h"

using namespace std;

namespace Ast {
struct Node;
struct ExecutableFnNode;
struct ExprValue;
}  // namespace Ast

struct Frame {
  unordered_map<string, Ast::ExprValue> variables{};
};

struct Line {
  Vector2 from;
  Vector2 to;
};

struct VM {
  Vector2 pos{};
  float angle = 0.0f;
  bool isDown = true;

  vector<Frame> frames{};
  vector<Line> history{};
  unordered_map<string, shared_ptr<Ast::ExecutableFnNode>> functions{};

  VM() {}

  void reset() {
    frames.clear();
    frames.emplace_back();

    history.clear();

    functions.clear();

    angle = 0.0f;
    isDown = true;
    pos.x = GetScreenWidth() >> 1;
    pos.y = GetScreenHeight() >> 1;
  }

  void forward(float v) {
    Vector2 prevPos{pos};

    pos.x += sinf(rad()) * v;
    pos.y += cosf(rad()) * -v;

    if (isDown) {
      history.emplace_back(prevPos, pos);
    }
  }

  void backward(float v) { forward(-v); }

  void left(float d) {
    angle -= d;
    normalizeAngle();
  }

  void right(float d) { left(-d); }

  void setPos(float x, float y) {
    pos.x = x;
    pos.y = y;
  }

  float rad() const { return (angle / 180.0) * numbers::pi; }
  void normalizeAngle() { angle = fmod(fmod(angle, 360) + 360.0f, 360); }
};
