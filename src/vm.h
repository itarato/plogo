#pragma once

#include <algorithm>
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
  float thickness;
  Color color;
};

struct IntVar {
  int min;
  int max;
};

struct VM {
  Vector2 pos{};
  float angle = 0.0f;
  bool isDown = true;
  float thickness = 1.0;
  Color color = BLACK;

  vector<Frame> frames{};
  vector<Line> history{};
  unordered_map<string, shared_ptr<Ast::ExecutableFnNode>> functions{};

  unordered_map<string, IntVar> intVars{};

  VM() { frames.emplace_back(); }

  void reset() {
    // Leave base frame.
    // Risk: base frame is always kept as is - assuming that initialization of a
    // used variable must happen always. As well this keeps preset variables the
    // same at a cost of persisting state between resets.
    while (frames.size() > 1) frames.pop_back();
    assert(frames.size() == 1);

    history.clear();
    functions.clear();
    intVars.clear();

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
      history.emplace_back(prevPos, pos, thickness, color);
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
