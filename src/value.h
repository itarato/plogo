#pragma once

#include <exception>
#include <string>

#include "util.h"

using namespace std;

enum class ValueKind {
  String,
  Undefined,
  Number,
  Boolean,
};

struct Value {
  union {
    float floatVal;
    bool boolVal;
    char *strVal;
  };

  ValueKind kind;

  Value() : floatVal(0.0), kind(ValueKind::Undefined) {
  }

  explicit Value(float v) : floatVal(v), kind(ValueKind::Number) {
  }

  explicit Value(bool v) : boolVal(v), kind(ValueKind::Boolean) {
  }

  explicit Value(string const &v) : kind(ValueKind::String) {
    strVal = new char[v.length() + 1];
    strcpy(strVal, v.c_str());
  }

  Value &operator=(Value const &other) {
    if (this != &other) {
      cleanup();
      copy_from(other);
    }

    return *this;
  }

  Value(Value const &other) : kind(ValueKind::Undefined) {
    copy_from(other);
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
        DEBUG("%s", strVal);
        break;
      default:
        DEBUG("NULL");
        break;
    };
  }

  ~Value() {
    cleanup();
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

 private:
  void cleanup() {
    if (kind == ValueKind::String) delete[] strVal;
  }

  void copy_from(Value const &other) {
    kind = other.kind;
    switch (other.kind) {
      case ValueKind::Boolean:
        boolVal = other.boolVal;
        break;
      case ValueKind::Number:
        floatVal = other.floatVal;
        break;
      case ValueKind::String:
        strVal = new char[strlen(other.strVal) + 1];
        strcpy(strVal, other.strVal);
        break;
      default:
        floatVal = 0.0f;
        break;
    };
  }
};
