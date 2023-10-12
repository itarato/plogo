#pragma once

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

#include "raylib.h"

using namespace std;

struct IntVector2 {
  int x = 0;
  int y = 0;
};

template <typename T>
inline T toRange(T v, T vmin, T vmax) {
  v = min(v, vmax);
  v = max(v, vmin);
  return v;
}
