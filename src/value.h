#pragma once

#include <string>

#include "util.h"

using namespace std;

enum class ValueKind {
  String,
  Number,
  Boolean,
  Unknown,
};

struct Value {
  union {
    float floatVal;
    bool boolVal;
    string strVal;
  };
  ValueKind kind;

  Value() : floatVal(0.0), kind(ValueKind::Unknown) {}
  Value(float v) : floatVal(v), kind(ValueKind::Number) {}
  Value(bool v) : boolVal(v), kind(ValueKind::Boolean) {}
  Value(string v) : strVal(v), kind(ValueKind::String) {}

  Value &operator=(const Value &other) {
    kind = other.kind;
    switch (other.kind) {
      case ValueKind::Boolean:
        boolVal = other.boolVal;
        break;
      case ValueKind::Number:
        floatVal = other.floatVal;
        break;
      case ValueKind::String:
        strVal = other.strVal;
        break;
      default:
        floatVal = 0.0f;
        break;
    };

    return *this;
  }

  Value(const Value &other) { *this = other; }

  ~Value() {
    if (kind == ValueKind::String) strVal.~string();
  }

  Value add(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) PANIC("'add' on non numbers");
    return Value(floatVal + other.floatVal);
  }

  Value sub(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) PANIC("'sub' on non numbers");
    return Value(floatVal - other.floatVal);
  }

  Value mul(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) PANIC("'mul' on non numbers");
    return Value(floatVal * other.floatVal);
  }

  Value div(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) PANIC("'div' on non numbers");
    return Value(floatVal / other.floatVal);
  }

  Value lt(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) PANIC("'lt 'on non numbers");
    return Value(floatVal < other.floatVal);
  }

  inline bool is_same_kind(Value &other, ValueKind assertedKind) {
    return kind == assertedKind && other.kind == assertedKind;
  }
};
