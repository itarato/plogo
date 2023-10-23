#pragma once

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

#include "raylib.h"

#define INFO(...) log("\x1b[90mINFO\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)
#define WARN(...) log("\x1b[93mWARN\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)
#define DEBUG(...) log("\x1b[94mDEBG\x1b[0m", __FILE__, __LINE__, __VA_ARGS__)

#define THROW(...) throw_runtime_error(__VA_ARGS__)

using namespace std;

void log_va_list(const char* level, const char* fileName, int lineNo,
                 const char* s, va_list args) {
  printf("[%s][\x1b[93m%s\x1b[39m:\x1b[96m%d\x1b[0m] \x1b[94m", level, fileName,
         lineNo);
  vprintf(s, args);
  printf("\x1b[0m\n");
}

void log(const char* level, const char* fileName, int lineNo, const char* s,
         ...) {
  va_list args;
  va_start(args, s);
  log_va_list(level, fileName, lineNo, s, args);
  va_end(args);
}

void throw_runtime_error(const char* s, ...) {
  va_list args;
  va_start(args, s);

  char msgbuf[128];
  sprintf(msgbuf, s, args);

  va_end(args);

  throw runtime_error(msgbuf);
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

bool eqf(float a, float b, float epsilon = 0.005f) {
  return (fabs(a - b) < epsilon);
}

float randf() { return (float)(rand() & 0xFFFF) / 0xFFFF; }
float randf(float min, float max) { return randf() * (max - min) + min; }

inline void assert_or_throw(bool cond, string msg) {
  if (!cond) throw runtime_error(msg);
}
