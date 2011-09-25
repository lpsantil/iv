#ifndef IV_LV5_RUNTIME_NUMBER_H_
#define IV_LV5_RUNTIME_NUMBER_H_
#include <cstdlib>
#include "detail/array.h"
#include "detail/cstdint.h"
#include "conversions.h"
#include "platform_math.h"
#include "lv5/error_check.h"
#include "lv5/constructor_check.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/jsobject.h"
#include "lv5/jsnumberobject.h"
#include "lv5/jsstring.h"
#include "lv5/jsdtoa.h"
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.7.1.1 Number([value])
// section 15.7.2.1 new Number([value])
inline JSVal NumberConstructor(const Arguments& args, Error* e) {
  if (args.IsConstructorCalled()) {
    double res = 0.0;
    if (!args.empty()) {
      res = args.front().ToNumber(args.ctx(), IV_LV5_ERROR(e));
    }
    return JSNumberObject::New(args.ctx(), res);
  } else {
    if (args.empty()) {
      return JSVal::Int32(0);
    } else {
      return args.front().ToNumber(args.ctx(), e);
    }
  }
}

// section 15.7.4.2 Number.prototype.toString([radix])
inline JSVal NumberToString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toString", args, e);
  const JSVal& obj = args.this_binding();
  double num;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      num = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toString is not generic function");
      return JSEmpty;
    }
  } else {
    num = obj.number();
  }
  if (!args.empty()) {
    const JSVal& first = args[0];
    double radix;
    if (first.IsUndefined()) {
      radix = 10;
    } else {
      radix = first.ToNumber(args.ctx(), IV_LV5_ERROR(e));
      radix = core::DoubleToInteger(radix);
    }
    if (2 <= radix && radix <= 36) {
      // if radix == 10, through to radix 10 or no radix
      if (radix != 10) {
        if (core::IsNaN(num)) {
          return JSString::NewAsciiString(args.ctx(), "NaN");
        } else if (core::IsInf(num)) {
          if (num > 0) {
            return JSString::NewAsciiString(args.ctx(), "Infinity");
          } else {
            return JSString::NewAsciiString(args.ctx(), "-Infinity");
          }
        }
        JSStringBuilder builder;
        core::DoubleToStringWithRadix(num,
                                      static_cast<int>(radix),
                                      std::back_inserter(builder));
        return builder.Build(args.ctx());
      }
    } else {
      e->Report(Error::Range, "illegal radix");
      return JSEmpty;
    }
  }
  // radix 10 or no radix
  std::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(num,
                                                buffer.data(),
                                                buffer.size());
  return JSString::NewAsciiString(args.ctx(), str);
}

// section 15.7.4.2 Number.prototype.toLocaleString()
inline JSVal NumberToLocaleString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toLocaleString", args, e);
  const JSVal& obj = args.this_binding();
  double num;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      num = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toLocaleString is not generic function");
      return JSEmpty;
    }
  } else {
    num = obj.number();
  }
  std::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(num,
                                                buffer.data(),
                                                buffer.size());
  return JSString::NewAsciiString(args.ctx(), str);
}

// section 15.7.4.4 Number.prototype.valueOf()
inline JSVal NumberValueOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.valueOf", args, e);
  const JSVal& obj = args.this_binding();
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      return static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.valueOf is not generic function");
      return JSEmpty;
    }
  } else {
    return obj.number();
  }
}

// section 15.7.4.5 Number.prototype.toFixed(fractionDigits)
inline JSVal NumberToFixed(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toFixed", args, e);
  Context* const ctx = args.ctx();
  double fd;
  if (args.empty()) {
    fd = 0.0;
  } else {
    const JSVal& first = args[0];
    if (first.IsUndefined()) {
      fd = 0.0;
    } else {
      fd = first.ToNumber(ctx, IV_LV5_ERROR(e));
      fd = core::DoubleToInteger(fd);
    }
  }
  if (fd < 0 || fd > 20) {
    e->Report(Error::Range,
              "fractionDigits is in range between 0 to 20");
    return JSEmpty;
  }
  const JSVal& obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toFixed is not generic function");
      return JSEmpty;
    }
  } else {
    x = obj.number();
  }
  if (!(std::fabs(x) < 1e+21)) {
    // included NaN and Infinity
    std::array<char, 80> buffer;
    const char* const str = core::DoubleToCString(x,
                                                  buffer.data(),
                                                  buffer.size());
    return JSString::NewAsciiString(args.ctx(), str);
  } else {
    JSStringDToA builder(ctx);
    return builder.BuildFixed(x, core::DoubleToInt32(fd), 0);
  }
}

// section 15.7.4.6 Number.prototype.toExponential(fractionDigits)
inline JSVal NumberToExponential(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toExponential", args, e);
  Context* const ctx = args.ctx();

  const JSVal& obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toExponential is not generic function");
      return JSEmpty;
    }
  } else {
    x = obj.number();
  }

  if (core::IsNaN(x)) {
    return JSString::NewAsciiString(ctx, "NaN");
  } else if (core::IsInf(x)) {
    if (x < 0) {
      return JSString::NewAsciiString(ctx, "-Infinity");
    } else {
      return JSString::NewAsciiString(ctx, "Infinity");
    }
  }

  JSVal fractionDigits;
  double fd;
  if (args.empty()) {
    fd = 0.0;
  } else {
    fractionDigits = args[0];
    if (fractionDigits.IsUndefined()) {
      fd = 0.0;
    } else {
      fd = fractionDigits.ToNumber(ctx, IV_LV5_ERROR(e));
      fd = core::DoubleToInteger(fd);
    }
  }
  if (!fractionDigits.IsUndefined() &&
      (fd < 0 || fd > 20)) {
    e->Report(Error::Range,
              "fractionDigits is in range between 0 to 20");
    return JSEmpty;
  }
  const int f = core::DoubleToInt32(fd);

  JSStringDToA builder(ctx);
  if (fractionDigits.IsUndefined()) {
    return builder.BuildStandardExponential(x);
  } else {
    return builder.BuildExponential(x, f, 1);
  }
}

// section 15.7.4.7 Number.prototype.toPrecision(precision)
inline JSVal NumberToPrecision(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toPrecision", args, e);
  Context* const ctx = args.ctx();

  const JSVal& obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toPrecision is not generic function");
      return JSEmpty;
    }
  } else {
    x = obj.number();
  }

  double p;
  if (args.empty()) {
    return obj.ToString(ctx, e);
  } else {
    const JSVal& precision = args[0];
    if (precision.IsUndefined()) {
      return obj.ToString(ctx, e);
    } else {
      p = precision.ToNumber(ctx, IV_LV5_ERROR(e));
      p = core::DoubleToInteger(p);
    }
  }

  if (core::IsNaN(x)) {
    return JSString::NewAsciiString(ctx, "NaN");
  } else if (core::IsInf(x)) {
    if (x < 0) {
      return JSString::NewAsciiString(ctx, "-Infinity");
    } else {
      return JSString::NewAsciiString(ctx, "Infinity");
    }
  }

  if (p < 1 || p > 21) {
    e->Report(Error::Range,
              "precision is in range between 1 to 21");
    return JSEmpty;
  }

  JSStringDToA builder(ctx);
  return builder.BuildPrecision(x, core::DoubleToInt32(p), 0);
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_NUMBER_H_
