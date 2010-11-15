#include <iostream>  // NOLINT
#include "ast.h"
#include "jsenv.h"
#include "jsfunction.h"
#include "jsval.h"
#include "jsobject.h"
#include "interpreter.h"
#include "context.h"
#include "arguments.h"
#include "jsscript.h"
namespace iv {
namespace lv5 {

void JSFunction::Initialize(Context* ctx) {
  const Class& cls = ctx->Cls("Function");
  set_cls(cls.name);
  set_prototype(cls.prototype);
}

JSCodeFunction::JSCodeFunction(Context* ctx,
                               const FunctionLiteral* func,
                               JSScript* script,
                               JSEnv* env)
  : function_(func),
    script_(script),
    env_(env) {
  DefineOwnProperty(
      ctx, ctx->length_symbol(),
      DataDescriptor(static_cast<double>(func->params().size()),
                     PropertyDescriptor::NONE),
                     false, NULL);
}

JSVal JSCodeFunction::Call(
    const Arguments& args,
    Error* error) {
  Interpreter* const interp = args.interpreter();
  Context* const ctx = args.ctx();
  interp->CallCode(this, args, error);
  if (ctx->mode() == Context::RETURN) {
    ctx->set_mode(Context::NORMAL);
  }
  return ctx->ret();
}

core::UStringPiece JSCodeFunction::GetSource() const {
  const std::size_t start_pos = function_->start_position();
  const std::size_t end_pos = function_->end_position();
  return script_->source()->SubString(start_pos,
                                      end_pos - start_pos + 1);
}

bool JSFunction::HasInstance(Context* ctx,
                             const JSVal& val, Error* error) {
  if (!val.IsObject()) {
    return false;
  }
  const JSVal got = Get(ctx, ctx->prototype_symbol(), error);
  if (*error) {
    return false;
  }
  if (!got.IsObject()) {
    error->Report(Error::Type, "\"prototype\" is not object");
    return false;
  }
  const JSObject* const proto = got.object();
  const JSObject* obj = val.object()->prototype();
  while (obj) {
    if (obj == proto) {
      return true;
    } else {
      obj = obj->prototype();
    }
  }
  return false;
}

JSVal JSFunction::Get(Context* ctx,
                      Symbol name, Error* error) {
  const JSVal val = JSObject::Get(ctx, name, error);
  if (*error) {
    return val;
  }
  if (name == ctx->caller_symbol() &&
      val.IsCallable() &&
      val.object()->AsCallable()->IsStrict()) {
    error->Report(Error::Type,
                  "\"caller\" property is not accessible in strict code");
    return JSFalse;
  }
  return val;
}

JSNativeFunction::JSNativeFunction(Context* ctx, value_type func, std::size_t n)
  : func_(func) {
  DefineOwnProperty(
      ctx, ctx->length_symbol(),
      DataDescriptor(static_cast<double>(n),
                     PropertyDescriptor::NONE),
                     false, NULL);
}

JSVal JSNativeFunction::Call(const Arguments& args,
                             Error* error) {
  return func_(args, error);
}


void JSNativeFunction::Initialize(Context* ctx,
                                  value_type func, std::size_t n) {
  func_ = func;
  DefineOwnProperty(
      ctx, ctx->length_symbol(),
      DataDescriptor(static_cast<double>(n),
                     PropertyDescriptor::NONE),
                     false, NULL);
  JSFunction::Initialize(ctx);
}

} }  // namespace iv::lv5
