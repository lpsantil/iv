#ifndef _IV_LV5_CONTEXT_UTILS_H_
#define _IV_LV5_CONTEXT_UTILS_H_
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "lv5/class.h"
#include "lv5/symbol.h"
#include "lv5/jsast.h"
#include "lv5/vm_stack.h"

class Context;
class JSRegExpImpl;

namespace iv {
namespace lv5 {
namespace context {

const core::UString& GetSymbolString(const Context* ctx, const Symbol& sym);
const Class& Cls(Context* ctx, const Symbol& name);
const Class& Cls(Context* ctx, const core::StringPiece& str);

const Class& Cls(Context* ctx, const core::StringPiece& str);

Symbol Intern(Context* ctx, const core::StringPiece& str);
Symbol Intern(Context* ctx, const core::UStringPiece& str);
Symbol Intern(Context* ctx, const Identifier& ident);
Symbol Intern(Context* ctx, uint32_t index);
Symbol Intern(Context* ctx, double number);

Symbol Lookup(Context* ctx, const core::StringPiece& str, bool* res);
Symbol Lookup(Context* ctx, const core::UStringPiece& str, bool* res);
Symbol Lookup(Context* ctx, uint32_t index, bool* res);
Symbol Lookup(Context* ctx, double number, bool* res);

Symbol constructor_symbol(const Context* ctx);
Symbol prototype_symbol(const Context* ctx);
Symbol length_symbol(const Context* ctx);

Error* error(Context* ctx);

bool IsStrict(const Context* ctx);

VMStack* stack(Context* ctx);

void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg);

} } }  // namespace iv::lv5::context
#endif  // _IV_LV5_CONTEXT_UTILS_H_
