#pragma once

#include "raylib.h"

struct Config {
  int win_w;
  int win_h;
  int liveToolbarWidth{400};

  int VirtualWindowWidth() const {
    return GetScreenWidth() - liveToolbarWidth;
  }

  int VirtualWindowHeight() const {
    return GetScreenHeight();
  }
} config;
