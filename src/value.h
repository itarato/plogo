#pragma once

#include <exception>
#include <string>

#include "util.h"

using namespace std;

enum class ValueKind {
  String,
  Number,
  Boolean,
  Undefined,
};

struct Value {
  union {
    float floatVal;
    bool boolVal;
    string strVal;
  };
  ValueKind kind;

  Value() : floatVal(0.0), kind(ValueKind::Undefined) {}
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

  void debug() const {
    switch (kind) {
      case ValueKind::Boolean:
        DEBUG("%b", boolVal);
        break;
      case ValueKind::Number:
        DEBUG("%f", floatVal);
        break;
      case ValueKind::String:
        DEBUG("%s", strVal.c_str());
        break;
      default:
        DEBUG("NULL");
        break;
    };
  }

  Value(const Value &other) { *this = other; }

  ~Value() {
    if (kind == ValueKind::String) strVal.~string();
  }

  Value add(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) {
      throw runtime_error("'add' on non numbers");
    }
    return Value(floatVal + other.floatVal);
  }

  Value sub(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) {
      throw runtime_error("'sub' on non numbers");
    }
    return Value(floatVal - other.floatVal);
  }

  Value mul(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) {
      throw runtime_error("'mul' on non numbers");
    }
    return Value(floatVal * other.floatVal);
  }

  Value div(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) {
      throw runtime_error("'div' on non numbers");
    }
    return Value(floatVal / other.floatVal);
  }

  Value lt(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) {
      throw runtime_error("'lt' on non numbers");
    }
    return Value(floatVal < other.floatVal);
  }

  Value lte(Value &other) {
    if (!is_same_kind(other, ValueKind::Number)) {
      throw runtime_error("'lte' on non numbers");
    }
    bool result = (floatVal < other.floatVal) || eqf(floatVal, other.floatVal);
    return Value(result);
  }

  Value eq(Value &other) {
    if (is_same_kind(other, ValueKind::Number)) {
      return Value(eqf(floatVal, other.floatVal));
    }
    if (is_same_kind(other, ValueKind::String)) {
      return Value(strVal == other.strVal);
    }
    if (is_same_kind(other, ValueKind::Boolean)) {
      return Value(boolVal == other.boolVal);
    }

    throw runtime_error("'eq' on non numbers");
    return Value{};
  }

  inline bool is_same_kind(Value &other, ValueKind assertedKind) {
    return kind == assertedKind && other.kind == assertedKind;
  }
};
