#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "raylib.h"
#include "value.h"

using namespace std;

namespace Ast {
struct ExecutableFnNode;
}  // namespace Ast

struct Frame {
  unordered_map<string, Value> variables{};
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

struct FloatVar {
  float min;
  float max;
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
  unordered_map<string, FloatVar> floatVars{};
  vector<Value> stack{};

  VM() { frames.emplace_back(); }

  void hardReset() {
    frames.clear();
    frames.emplace_back();

    reset();
  }

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
    floatVars.clear();
    stack.clear();

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
