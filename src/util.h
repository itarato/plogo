#pragma once

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

#include "raylib.h"

#define FAIL(...) log("\x1b[91mFAIL\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)
#define PASS(...) log("\x1b[92mPASS\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...) log("\x1b[90mINFO\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)

#define PANIC(...) \
  panic("\x1b[91mPANIC\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)

#define ASSERT(exp, msg) (exp ? PASS(msg) : FAIL(msg))

using namespace std;

void log(const char* level, const char* fileName, int lineNo, const char* s,
         ...) {
  va_list args;
  va_start(args, s);

  printf("[%s][\x1b[93m%s\x1b[39m:\x1b[96m%d\x1b[0m] \x1b[94m", level, fileName,
         lineNo);
  vprintf(s, args);
  printf("\x1b[0m\n");

  va_end(args);
}

void panic(const char* level, const char* fileName, int lineNo, const char* s,
           ...) {
  va_list args;
  va_start(args, s);

  log(level, fileName, lineNo, s, args);

  va_end(args);

  exit(EXIT_FAILURE);
}

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

void panic(string msg) {
  printf("ERROR: %s", msg.c_str());
  exit(EXIT_FAILURE);
}

bool eqf(float a, float b, float epsilon = 0.005f) {
  return (fabs(a - b) < epsilon);
}

float randf() { return (float)(rand() & 0xFFFF) / 0xFFFF; }
float randf(float min, float max) { return randf() * (max - min) + min; }
