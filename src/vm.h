#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <mutex>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "config.h"
#include "raylib.h"
#include "util.h"
#include "value.h"

using namespace std;

constexpr int STACK_LIMIT = 255;
constexpr int HISTORY_LIMIT = 1000000;

namespace Ast {
struct ExecutableFnNode;
}  // namespace Ast

struct Frame {
  unordered_map<string, Value> variables{};
  int loopCount{0};
};

struct Line {
  Vector2 from;
  Vector2 to;
  float thickness;
  Color color;

  Line(Vector2 from, Vector2 to, float thickness, Color color)
      : from(from), to(to), thickness(thickness), color(color) {
  }
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
  Color backgroundColor = RAYWHITE;
  std::string lastCompileError{};

  unordered_map<string, shared_ptr<Ast::ExecutableFnNode>> functions{};

  unordered_map<string, IntVar> intVars{};
  unordered_map<string, FloatVar> floatVars{};
  vector<Value> stack{};

  VM() {
    frames.emplace_back();
  }

  void reset(bool withHardReset = false, bool clearState = true) {
    if (withHardReset) {
      frames.clear();
      frames.emplace_back();
    }

    // Leave base frame.
    // Risk: base frame is always kept as is - assuming that initialization of a
    // used variable must happen always. As well this keeps preset variables the
    // same at a cost of persisting state between resets.
    while (frames.size() > 1) frames.pop_back();
    assert(frames.size() == 1);

    functions.clear();
    intVars.clear();
    floatVars.clear();
    stack.clear();
    color = BLACK;
    thickness = 1.0;

    if (clearState) {
      history.clear();
      angle = 0.0f;
      isDown = true;
      pos.x = config.VirtualWindowWidth() >> 1;
      pos.y = config.VirtualWindowHeight() >> 1;
    }
  }

  void forward(float v) {
    Vector2 prevPos{pos};

    pos.x += sinf(rad()) * v;
    pos.y += cosf(rad()) * -v;

    if (isDown) add_line(Line(prevPos, pos, thickness, color));
  }

  void backward(float v) {
    forward(-v);
  }

  void left(float d) {
    angle -= d;
    normalizeAngle();
  }

  void right(float d) {
    left(-d);
  }

  void setPos(float x, float y) {
    pos.x = x;
    pos.y = y;
  }

  float rad() const {
    return angle * DEG2RAD;
  }

  void normalizeAngle() {
    angle = fmod(fmod(angle, 360) + 360.0f, 360);
  }

  Frame& top_frame() {
    return frames.front();
  }

  Frame& bottom_frame() {
    return frames.back();
  }

  [[nodiscard]] bool try_push_frame(Frame&& frame) {
    if (frames.size() >= STACK_LIMIT) return false;

    frames.push_back(std::move(frame));
    return true;
  }

  void pop_frame() {
    frames.pop_back();
  }

  constexpr std::size_t history_size() const {
    return history.size();
  }

  void add_line(Line&& line) {
    if (history.size() > HISTORY_LIMIT) THROW("Too many lines");

    history.push_back(std::move(line));
  }

  constexpr std::vector<Line> const& get_history() const {
    return history;
  }

 private:
  vector<Frame> frames{};
  vector<Line> history{};
};
