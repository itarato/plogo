#pragma once

#include <cmath>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

#include "raylib.h"

using namespace std;
struct Frame {
  unordered_map<string, float> variables{};
};

struct VM {
  Vector2 pos{};
  float angle = 0.0f;
  bool isDown = true;

  vector<Frame> frames{};

  VM() {}

  void reset() {
    frames.clear();
    frames.emplace_back();

    angle = 0.0f;
    isDown = true;
    pos.x = GetScreenWidth() >> 1;
    pos.y = GetScreenHeight() >> 1;
  }

  void forward(float v) {
    Vector2 prevPos{pos};

    pos.x += sinf(rad()) * v;
    pos.y += cosf(rad()) * v;

#ifndef NORAYLIB
    DrawLine(prevPos.x, prevPos.y, pos.x, pos.y, BLACK);
#endif
  }

  void backward(float v) { forward(-v); }

  void left(float d) {
    angle -= d;
    normalizeAngle();
  }

  void right(float d) { left(-d); }

  float rad() const { return (angle / 180.0) * numbers::pi; }
  void normalizeAngle() { angle = fmod(fmod(angle, 360) + 360.0f, 360); }
};
