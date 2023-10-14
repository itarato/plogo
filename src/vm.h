#pragma once

#include <cmath>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

#include "raylib.h"

using namespace std;
struct Frame {
  unordered_map<string, float> variables;
};

struct VM {
  Vector2 pos{};
  float angle = 0.0f;
  bool isDown = true;

  vector<Frame> frames{};

  VM() : frames({Frame{}}) {}

  void forward(float v) {
    Vector2 currentPos{pos};

    pos.x += sinf(rad()) * v;
    pos.y += cosf(rad()) * v;

    DrawLine(currentPos.x, currentPos.y, pos.x, pos.y, BLACK);
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
