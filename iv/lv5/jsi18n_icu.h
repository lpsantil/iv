#ifndef IV_LV5_JSI18N_ICU_H_
#define IV_LV5_JSI18N_ICU_H_

// Currently, we implement them by ICU.
// So this is enabled when IV_ENABLE_I18N flag is enabled and ICU is provided.
#include <unicode/coll.h>
#include <unicode/decimfmt.h>
#include <unicode/datefmt.h>
#include <unicode/numsys.h>
#include <unicode/dtptngen.h>
#include <unicode/smpdtfmt.h>
#define IV_ICU_VERSION \
    (U_ICU_VERSION_MAJOR_NUM * 10000 + \
     U_ICU_VERSION_MINOR_NUM * 100 + \
     U_ICU_VERSION_PATCHLEVEL_NUM)

namespace iv {
namespace lv5 {

class ICUStringIteration
  : public std::iterator<std::forward_iterator_tag, const char*> {
 public:
  typedef ICUStringIteration this_type;

  explicit ICUStringIteration(icu::StringEnumeration* enumeration)
    : enumeration_(enumeration) {
    Next();
  }

  ICUStringIteration()
    : enumeration_(nullptr),
      current_(nullptr) {
  }

  ICUStringIteration(const ICUStringIteration& rhs)
    : enumeration_((rhs.enumeration_) ? rhs.enumeration_->clone() : nullptr),
      current_(rhs.current_) {
  }

  this_type& operator++() {
    Next();
    return *this;
  }

  this_type operator++(int) {  // NOLINT
    const this_type temp(*this);
    Next();
    return temp;
  }

  const char* operator*() const {
    return current_;
  }

  this_type& operator=(const this_type& rhs) {
    this_type tmp(rhs);
    tmp.swap(*this);
    return *this;
  }

  bool operator==(const this_type& rhs) const {
    if (enumeration_) {
      if (!rhs.enumeration_) {
        return false;
      }
      return *enumeration_ == *rhs.enumeration_;
    } else {
      return enumeration_ == rhs.enumeration_;
    }
  }

  bool operator!=(const this_type& rhs) const {
      return !(operator==(rhs));
  }

  friend void swap(this_type& lhs, this_type& rhs) {
    using std::swap;
    swap(lhs.enumeration_, rhs.enumeration_);
    swap(lhs.current_, rhs.current_);
  }

  void swap(this_type& rhs) {
    using std::swap;
    swap(*this, rhs);
  }

 private:
  void Next() {
    if (enumeration_) {
      UErrorCode status = U_ZERO_ERROR;
      current_ = enumeration_->next(nullptr, status);
      if (U_FAILURE(status)) {
        current_ = nullptr;
      }
      if (!current_) {
        enumeration_.reset();
      }
    }
  }

  std::unique_ptr<icu::StringEnumeration> enumeration_;
  const char* current_;
};

class JSCollator : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSCollator, Collator)

  enum CollatorField {
    USAGE = 0,
    NUMERIC,
    NORMALIZATION,
    CASE_FIRST,
    SENSITIVITY,
    IGNORE_PUNCTUATION,
    BOUND_COMPARE,
    LOCALE,
    COLLATION,
    NUM_OF_FIELDS
  };

  explicit JSCollator(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      collator_fields_(),
      collator_(new i18n::GCHandle<icu::Collator>()) {
  }

  JSCollator(Context* ctx, Map* map)
    : JSObject(map),
      collator_fields_(),
      collator_(new i18n::GCHandle<icu::Collator>()) {
  }

  static JSCollator* New(Context* ctx) {
    JSCollator* const collator = new JSCollator(ctx);
    collator->set_cls(JSCollator::GetClass());
    collator->set_prototype(
        context::GetClassSlot(ctx, Class::Collator).prototype);
    return collator;
  }

  static JSCollator* NewPlain(Context* ctx, Map* map) {
    JSCollator* obj = new JSCollator(ctx, map);
    Error::Dummy e;
    JSCollator::Initialize(ctx, obj, JSUndefined, JSUndefined, &e);
    return obj;
  }

  JSVal GetField(std::size_t n) {
    assert(n < collator_fields_.size());
    return collator_fields_[n];
  }

  void SetField(std::size_t n, JSVal v) {
    assert(n < collator_fields_.size());
    collator_fields_[n] = v;
  }

  void set_collator(icu::Collator* collator) {
    collator_->handle.reset(collator);
  }

  icu::Collator* collator() const { return collator_->handle.get(); }

  JSVal Compare(Context* ctx, JSVal left, JSVal right, Error* e) {
    const std::u16string lhs = left.ToU16String(ctx, IV_LV5_ERROR(e));
    const std::u16string rhs = right.ToU16String(ctx, IV_LV5_ERROR(e));
    const UChar* lstr = reinterpret_cast<const UChar*>(lhs.data());
    const UChar* rstr = reinterpret_cast<const UChar*>(rhs.data());
    UErrorCode status = U_ZERO_ERROR;
    UCollationResult result =
        collator()->compare(lstr, lhs.size(), rstr, rhs.size(), status);
    if (U_FAILURE(status)) {
      e->Report(Error::Type, "collator compare failed");
      return JSEmpty;
    }
    return JSVal::Int32(static_cast<int32_t>(result));
  }

  static JSVal Initialize(Context* ctx,
                          JSCollator* obj,
                          JSVal requested_locales,
                          JSVal op,
                          Error* e);

 private:
  std::array<JSVal, NUM_OF_FIELDS> collator_fields_;
  i18n::GCHandle<icu::Collator>* collator_;
};

class JSCollatorBoundFunction : public JSFunction {
 public:
  virtual JSVal Call(Arguments* args, JSVal this_binding, Error* e) {
    return collator_->Compare(args->ctx(), args->At(0), args->At(1), e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    e->Report(Error::Type,
              "collator bound function does not have [[Construct]]");
    return JSEmpty;
  }

  virtual JSAPI NativeFunction() const { return nullptr; }

  static JSCollatorBoundFunction* New(Context* ctx, JSCollator* collator) {
    JSCollatorBoundFunction* const obj =
        new JSCollatorBoundFunction(ctx, collator);
    obj->Initialize(ctx);
    return obj;
  }

 private:
  explicit JSCollatorBoundFunction(Context* ctx, JSCollator* collator)
    : JSFunction(ctx, FUNCTION_NATIVE, false),
      collator_(collator) {
    Error::Dummy dummy;
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(JSVal::UInt32(2u), ATTR::NONE), false, nullptr);
    DefineOwnProperty(
        ctx, symbol::name(),
        DataDescriptor(
            JSString::NewAsciiString(ctx, "compare", &dummy),
            ATTR::NONE), false, nullptr);
  }

  JSCollator* collator_;
};

class JSDateTimeFormat : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSDateTimeFormat, DateTimeFormat)

  enum DateTimeFormatField {
    CALENDAR = 0,
    TIME_ZONE,
    HOUR12,
    PATTERN,
    LOCALE,
    NUMBERING_SYSTEM,
    WEEKDAY,
    ERA,
    YEAR,
    MONTH,
    DAY,
    HOUR,
    MINUTE,
    SECOND,
    TIME_ZONE_NAME,
    FORMAT_MATCH,
    NUM_OF_FIELDS
  };

  explicit JSDateTimeFormat(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      date_time_format_(new i18n::GCHandle<icu::DateFormat>()),
      fields_() {
  }

  JSDateTimeFormat(Context* ctx, Map* map)
    : JSObject(map),
      date_time_format_(new i18n::GCHandle<icu::DateFormat>()),
      fields_() {
  }

  JSVal GetField(std::size_t n) {
    assert(n < fields_.size());
    return fields_[n];
  }

  void SetField(std::size_t n, JSVal v) {
    assert(n < fields_.size());
    fields_[n] = v;
  }

  void set_date_time_format(icu::DateFormat* format) {
    date_time_format_->handle.reset(format);
  }

  icu::DateFormat* date_time_format() const {
    return date_time_format_->handle.get();
  }

  JSVal Format(Context* ctx, double value, Error* e) {
    UErrorCode err = U_ZERO_ERROR;
    icu::UnicodeString result;
    date_time_format()->format(value, result);
    if (U_FAILURE(err)) {
      e->Report(Error::Type, "Intl.DateTimeFormat parse failed");
      return JSEmpty;
    }
    return JSString::New(
        ctx,
        result.getBuffer(),
        result.getBuffer() + result.length(), false, e);
  }

  static JSDateTimeFormat* New(Context* ctx) {
    JSDateTimeFormat* const collator = new JSDateTimeFormat(ctx);
    collator->set_cls(JSDateTimeFormat::GetClass());
    collator->set_prototype(
        context::GetClassSlot(ctx, Class::DateTimeFormat).prototype);
    return collator;
  }

  static JSDateTimeFormat* NewPlain(Context* ctx, Map* map) {
    JSDateTimeFormat* obj = new JSDateTimeFormat(ctx, map);
    Error::Dummy e;
    JSDateTimeFormat::Initialize(ctx, obj, JSUndefined, JSUndefined, &e);
    return obj;
  }

  static JSVal Initialize(Context* ctx,
                          JSDateTimeFormat* obj,
                          JSVal requested_locales,
                          JSVal op,
                          Error* e);


 private:
  i18n::GCHandle<icu::DateFormat>* date_time_format_;
  std::array<JSVal, NUM_OF_FIELDS> fields_;
};

namespace detail_i18n {

struct CollatorOption {
  typedef std::array<const char*, 4> Values;
  const char* key;
  std::size_t field;
  const char* property;
  Options::Type type;
  Values values;
};

typedef std::array<CollatorOption, 6> CollatorOptionTable;

static const CollatorOptionTable kCollatorOptionTable = { {
  { "kn", JSCollator::NUMERIC, "numeric",
    Options::BOOLEAN, { { nullptr } } },
  { "kk", JSCollator::NORMALIZATION, "normalization",
    Options::BOOLEAN, { { nullptr } } },
  { "kf", JSCollator::CASE_FIRST, "caseFirst",
    Options::STRING, { { "upper", "lower", "false", nullptr } } },
} };

template<JSCollator::CollatorField TYPE, UColAttribute ATTR>
inline void SetICUCollatorBooleanOption(JSCollator* obj,
                                        icu::Collator* collator, Error* e) {
  UErrorCode status = U_ZERO_ERROR;
  const bool res = obj->GetField(TYPE).ToBoolean();
  collator->setAttribute(ATTR, res ? UCOL_ON : UCOL_OFF, status);
  if (U_FAILURE(status)) {
    e->Report(Error::Type, "icu collator initialization failed");
    return;
  }
}

struct DateTimeOption {
  typedef std::array<const char*, 5> Values;
  const char* key;
  JSDateTimeFormat::DateTimeFormatField field;
  Values values;
};
typedef std::array<DateTimeOption, 9> DateTimeOptions;
static const DateTimeOptions kDateTimeOptions = { {
  { "weekday",      JSDateTimeFormat::WEEKDAY,
    { { "narrow", "short", "long", nullptr } } },
  { "era",          JSDateTimeFormat::ERA,
    { { "narrow", "short", "long", nullptr } } },
  { "year",         JSDateTimeFormat::YEAR,
    { { "2-digit", "numeric", nullptr } } },
  { "month",        JSDateTimeFormat::MONTH,
    { { "2-digit", "numeric", "narrow", "short", "long" } } },
  { "day",          JSDateTimeFormat::DAY,
    { { "2-digit", "numeric", nullptr } } },
  { "hour",         JSDateTimeFormat::HOUR,
    { { "2-digit", "numeric", nullptr } } },
  { "minute",       JSDateTimeFormat::MINUTE,
    { { "2-digit", "numeric", nullptr } } },
  { "second",       JSDateTimeFormat::SECOND,
    { { "2-digit", "numeric", nullptr } } },
  { "timeZoneName", JSDateTimeFormat::TIME_ZONE_NAME,
    { { "short", "long", nullptr } } }
} };

inline JSObject* BasicFormatMatch(Context* ctx,
                                  JSObject* options,
                                  JSObject* formats, Error* e) {
  const int32_t removal_penalty = 120;
  const int32_t addition_penalty = 20;
  const int32_t long_less_penalty = 8;
  const int32_t long_more_penalty = 6;
  const int32_t short_less_penalty = 6;
  const int32_t short_more_penalty = 3;

  int32_t best_score = INT32_MIN;
  JSObject* best_format = nullptr;
  const uint32_t len =
      internal::GetLength(ctx, formats, IV_LV5_ERROR(e));
  for (uint32_t i = 0; i < len; ++i) {
    const JSVal v =
        formats->Get(ctx, symbol::MakeSymbolFromIndex(i),
                     IV_LV5_ERROR(e));
    assert(v.IsObject());
    JSObject* format = v.object();
    int32_t score = 0;
    for (DateTimeOptions::const_iterator it = kDateTimeOptions.begin(),
         last = kDateTimeOptions.end(); it != last; ++it) {
      const Symbol name = ctx->Intern(it->key);
      const JSVal options_prop =
          options->Get(ctx, name, IV_LV5_ERROR(e));
      const PropertyDescriptor format_prop_desc =
          formats->GetOwnProperty(ctx, name);
      JSVal format_prop = JSUndefined;
      if (!format_prop_desc.IsEmpty()) {
        format_prop = formats->Get(ctx, name, IV_LV5_ERROR(e));
      }
      if (options_prop.IsUndefined() && !format_prop.IsUndefined()) {
        score -= addition_penalty;
      } else if (!options_prop.IsUndefined() && format_prop.IsUndefined()) {
        score -= removal_penalty;
      } else {
        typedef std::array<const char*, 5> ValueTypes;
        static const ValueTypes kValueTypes = { {
          "2-digit", "numeric", "narrow", "short", "long"
        } };
        int32_t options_prop_index = -1;
        if (options_prop.IsString()) {
          const std::string str = options_prop.string()->GetUTF8();
          const ValueTypes::const_iterator it =
              std::find(kValueTypes.begin(), kValueTypes.end(), str);
          if (it != kValueTypes.end()) {
            options_prop_index = std::distance(kValueTypes.begin(), it);
          }
        }
        int32_t format_prop_index = -1;
        if (format_prop.IsString()) {
          const std::string str = format_prop.string()->GetUTF8();
          const ValueTypes::const_iterator it =
              std::find(kValueTypes.begin(), kValueTypes.end(), str);
          if (it != kValueTypes.end()) {
            format_prop_index = std::distance(kValueTypes.begin(), it);
          }
        }
        const int32_t delta =
            (std::max)(
                (std::min)(format_prop_index - options_prop_index, 2), -2);
        switch (delta) {
          case 2: {
            score -= long_more_penalty;
            break;
          }
          case 1: {
            score -= short_more_penalty;
            break;
          }
          case -1: {
            score -= short_less_penalty;
            break;
          }
          case -2: {
            score -= long_less_penalty;
            break;
          }
        }
      }
    }
    if (score > best_score) {
      best_score = score;
      best_format = format;
    }
  }
  return best_format;
}

inline JSObject* BestFitFormatMatch(Context* ctx,
                                    JSObject* options,
                                    JSObject* formats, Error* e) {
  return BasicFormatMatch(ctx, options, formats, e);
}

}  // namespace detail_i18n

// initializers

inline JSVal JSCollator::Initialize(Context* ctx,
                                    JSCollator* obj,
                                    JSVal req,
                                    JSVal op,
                                    Error* e) {
  JSVector* requested =
      i18n::CanonicalizeLocaleList(ctx, req, IV_LV5_ERROR(e));

  JSObject* options = nullptr;
  if (op.IsUndefined()) {
    options = JSObject::New(ctx);
  } else {
    options = op.ToObject(ctx, IV_LV5_ERROR(e));
  }

  detail_i18n::Options opt(options);

  {
    JSVector* vec = JSVector::New(ctx);
    JSString* sort = JSString::NewAsciiString(ctx, "sort", e);
    JSString* search = JSString::NewAsciiString(ctx, "search", e);
    vec->push_back(sort);
    vec->push_back(search);
    const JSVal u = opt.Get(ctx,
                            ctx->Intern("usage"),
                            detail_i18n::Options::STRING, vec,
                            sort, IV_LV5_ERROR(e));
    assert(u.IsString());
    obj->SetField(JSCollator::USAGE, u);
  }

  const core::i18n::LookupResult result =
      detail_i18n::ResolveLocale(
          ctx,
          ICUStringIteration(icu::NumberFormat::getAvailableLocales()),
          ICUStringIteration(),
          requested,
          options,
          IV_LV5_ERROR(e));
  JSString* locale_string =
      JSString::NewAsciiString(ctx, result.locale(), IV_LV5_ERROR(e));
  obj->SetField(JSCollator::LOCALE, locale_string);
  icu::Locale locale(result.locale().c_str());
  {
    for (detail_i18n::CollatorOptionTable::const_iterator
         it = detail_i18n::kCollatorOptionTable.begin(),
         last = detail_i18n::kCollatorOptionTable.end();
         it != last; ++it) {
      JSVector* vec = nullptr;
      if (it->values[0] != nullptr) {
        vec = JSVector::New(ctx);
        for (detail_i18n::CollatorOption::Values::const_iterator
             oit = it->values.begin(),
             olast = it->values.end();
             oit != olast && *oit; ++oit) {
          JSString* str = JSString::NewAsciiString(ctx, *oit, IV_LV5_ERROR(e));
          vec->push_back(str);
        }
      }
      JSVal value = opt.Get(ctx,
                            ctx->Intern(it->property),
                            it->type, vec,
                            JSUndefined, IV_LV5_ERROR(e));
      if (it->type == detail_i18n::Options::BOOLEAN) {
        if (!value.IsUndefined()) {
          if (!value.IsBoolean()) {
            const JSVal str = value.ToString(ctx, IV_LV5_ERROR(e));
            value = JSVal::Bool(
                JSVal::StrictEqual(ctx->global_data()->string_true(), str));
          }
        } else {
          value = JSFalse;
        }
      }
      obj->SetField(it->field, value);
    }
  }

  {
    // TODO(Constellation) lookup co from UnicodeExtensions
//    for (detail_i18n::CollatorOptionTable::const_iterator
//         it = detail_i18n::kCollatorOptionTable.begin(),
//         last = detail_i18n::kCollatorOptionTable.end();
//         it != last; ++it) {
//      const JSVal option_value = obj->GetField(it->field);
//      if (option_value.IsUndefined()) {
//        const detail_i18n::ExtensionMap::const_iterator
//            target = map.find(it->key);
//        if (target != map.end()) {
//          JSVal value;
//          if (it->type == detail_i18n::Options::BOOLEAN) {
//            obj->SetField(it->field,
//                          JSVal::Bool(target->second == "true"));
//          } else if (it->type == detail_i18n::Options::STRING) {
//            if (it->values[0]) {
//              for (const char* const * ptr = it->values.data(); *ptr; ++ptr) {
//                if (*ptr == target->second) {
//                  obj->SetField(
//                      it->field,
//                      JSString::NewAsciiString(ctx, target->second));
//                }
//              }
//            }
//          }
//        }
//      }
//    }
  }

  UErrorCode status = U_ZERO_ERROR;
  icu::Collator* collator = icu::Collator::createInstance(locale, status);
  if (U_FAILURE(status)) {
    e->Report(Error::Type, "collator creation failed");
    return JSEmpty;
  }
  obj->set_collator(collator);

  {
    JSVector* vec = JSVector::New(ctx);
    JSString* o_base = JSString::NewAsciiString(ctx, "base", e);
    JSString* o_accent = JSString::NewAsciiString(ctx, "accent", e);
    JSString* o_case = JSString::NewAsciiString(ctx, "case", e);
    JSString* o_variant = JSString::NewAsciiString(ctx, "variant", e);
    vec->push_back(o_base);
    vec->push_back(o_accent);
    vec->push_back(o_case);
    vec->push_back(o_variant);
    JSVal s = opt.Get(ctx,
                      ctx->Intern("sensitivity"),
                      detail_i18n::Options::STRING, vec,
                      JSUndefined, IV_LV5_ERROR(e));
    if (s.IsUndefined()) {
      if (JSVal::StrictEqual(obj->GetField(JSCollator::USAGE),
                             JSString::NewAsciiString(ctx, "sort", e))) {
        s = o_variant;
      } else {
        s = o_base;
      }
    }
    obj->SetField(JSCollator::SENSITIVITY, s);
  }

  {
    const JSVal ip = opt.Get(ctx,
                             ctx->Intern("ignorePunctuation"),
                             detail_i18n::Options::BOOLEAN, nullptr,
                             JSFalse, IV_LV5_ERROR(e));
    obj->SetField(JSCollator::IGNORE_PUNCTUATION, ip);
  }
  obj->SetField(JSCollator::BOUND_COMPARE, JSUndefined);

  // initialize icu::Collator
  detail_i18n::SetICUCollatorBooleanOption<
      JSCollator::NUMERIC,
      UCOL_NUMERIC_COLLATION>(obj, collator, IV_LV5_ERROR(e));
  detail_i18n::SetICUCollatorBooleanOption<
      JSCollator::NORMALIZATION,
      UCOL_NORMALIZATION_MODE>(obj, collator, IV_LV5_ERROR(e));

  {
    const JSVal v = obj->GetField(JSCollator::CASE_FIRST);
    if (v.IsUndefined()) {
      collator->setAttribute(UCOL_CASE_FIRST, UCOL_OFF, status);
    } else {
      assert(v.IsString());
      JSString* s = v.string();
      const std::string str(s->begin(), s->end());
      if (str == "upper") {
        collator->setAttribute(UCOL_CASE_FIRST, UCOL_UPPER_FIRST, status);
      } else if (str == "lower") {
        collator->setAttribute(UCOL_CASE_FIRST, UCOL_LOWER_FIRST, status);
      } else {
        collator->setAttribute(UCOL_CASE_FIRST, UCOL_OFF, status);
      }
    }
    if (U_FAILURE(status)) {
      e->Report(Error::Type, "icu collator initialization failed");
      return JSEmpty;
    }
  }

  {
    const JSVal v = obj->GetField(JSCollator::SENSITIVITY);
    if (v.IsUndefined()) {
      collator->setStrength(icu::Collator::TERTIARY);
    } else {
      assert(v.IsString());
      JSString* s = v.string();
      const std::string str(s->begin(), s->end());
      if (str == "base") {
        collator->setStrength(icu::Collator::PRIMARY);
      } else if (str == "accent") {
        collator->setStrength(icu::Collator::SECONDARY);
      } else if (str == "case") {
        collator->setStrength(icu::Collator::PRIMARY);
        collator->setAttribute(UCOL_CASE_LEVEL, UCOL_ON, status);
      } else {
        collator->setStrength(icu::Collator::TERTIARY);
      }
    }
    if (U_FAILURE(status)) {
      e->Report(Error::Type, "icu collator initialization failed");
      return JSEmpty;
    }
  }

  {
    if (obj->GetField(JSCollator::IGNORE_PUNCTUATION).ToBoolean()) {
      collator->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
      if (U_FAILURE(status)) {
        e->Report(Error::Type, "icu collator initialization failed");
        return JSEmpty;
      }
    }
  }
  return obj;
}

inline JSVal JSDateTimeFormat::Initialize(Context* ctx,
                                          JSDateTimeFormat* obj,
                                          JSVal req,
                                          JSVal op,
                                          Error* e) {
  JSVector* requested =
      i18n::CanonicalizeLocaleList(ctx, req, IV_LV5_ERROR(e));

  JSObject* options =
      detail_i18n::ToDateTimeOptions(ctx, op, true, false, IV_LV5_ERROR(e));

  detail_i18n::Options opt(options);

  int32_t num = 0;
  const icu::Locale* avail = icu::DateFormat::getAvailableLocales(num);
  std::vector<std::string> vec(num);
  for (int32_t i = 0; i < num; ++i) {
    vec[i] = avail[i].getName();
  }
  const core::i18n::LookupResult result =
      detail_i18n::ResolveLocale(
          ctx,
          vec.begin(),
          vec.end(),
          requested,
          JSObject::New(ctx),
          IV_LV5_ERROR(e));
  JSString* locale_string =
      JSString::NewAsciiString(ctx, result.locale(), IV_LV5_ERROR(e));
  obj->SetField(JSDateTimeFormat::LOCALE, locale_string);
  icu::Locale locale(result.locale().c_str());
  {
    UErrorCode err = U_ZERO_ERROR;
    std::unique_ptr<icu::NumberingSystem> numbering_system(
        icu::NumberingSystem::createInstance(locale, err));
    if (U_FAILURE(err)) {
      e->Report(Error::Type, "numbering system initialization failed");
      return JSEmpty;
    }
    JSString* ns =
        JSString::NewAsciiString(
            ctx, numbering_system->getName(), IV_LV5_ERROR(e));
    obj->SetField(JSDateTimeFormat::NUMBERING_SYSTEM, ns);
  }

  JSVal tz =
      options->Get(ctx, ctx->Intern("timeZone"), IV_LV5_ERROR(e));
  if (!tz.IsUndefined()) {
    JSString* str = tz.ToString(ctx, IV_LV5_ERROR(e));
    std::vector<char16_t> vec;
    for (JSString::const_iterator it = str->begin(),
         last = str->end(); it != last; ++it) {
      vec.push_back(core::i18n::ToLocaleIdentifierUpperCase(*it));
    }
    if (vec.size() != 3 || vec[0] != 'U' || vec[1] != 'T' || vec[2] != 'C') {
      e->Report(Error::Range, "tz is not UTC");
      return JSEmpty;
    }
    obj->SetField(JSDateTimeFormat::TIME_ZONE, str);
  }

  {
    const JSVal hour12 = opt.Get(ctx,
                                 ctx->Intern("hour12"),
                                 detail_i18n::Options::BOOLEAN, nullptr,
                                 JSUndefined, IV_LV5_ERROR(e));
    obj->SetField(JSDateTimeFormat::HOUR12, hour12);
  }

  {
    for (detail_i18n::DateTimeOptions::const_iterator
         it = detail_i18n::kDateTimeOptions.begin(),
         last = detail_i18n::kDateTimeOptions.end();
         it != last; ++it) {
      const Symbol prop = ctx->Intern(it->key);
      JSVector* vec = JSVector::New(ctx);
      for (detail_i18n::DateTimeOption::Values::const_iterator
           dit = it->values.begin(),
           dlast = it->values.end();
           dit != dlast && *dit; ++dit) {
        JSString* str = JSString::NewAsciiString(ctx, *dit, IV_LV5_ERROR(e));
        vec->push_back(str);
      }
      const JSVal value = opt.Get(ctx,
                                  prop,
                                  detail_i18n::Options::STRING,
                                  vec,
                                  JSUndefined, IV_LV5_ERROR(e));
      obj->SetField(it->field, value);
    }
  }

  {
    JSString* basic = JSString::NewAsciiString(ctx, "basic", e);
    JSString* best_fit = JSString::NewAsciiString(ctx, "best fit", e);
    JSVector* vec = JSVector::New(ctx);
    vec->push_back(basic);
    vec->push_back(best_fit);
    const JSVal matcher = opt.Get(ctx,
                                  ctx->Intern("formatMatch"),
                                  detail_i18n::Options::STRING,
                                  vec,
                                  best_fit, IV_LV5_ERROR(e));
    obj->SetField(JSDateTimeFormat::FORMAT_MATCH, matcher);
  }

  {
    const JSVal pattern =
        options->Get(ctx, ctx->Intern("pattern"), IV_LV5_ERROR(e));
    obj->SetField(JSDateTimeFormat::PATTERN, pattern);
  }

  // Generate skeleton
  // See http://userguide.icu-project.org/formatparse/datetime
  std::string buffer;
  {
    // weekday
    const JSVal val= obj->GetField(JSDateTimeFormat::WEEKDAY);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "narrow") {
        buffer.append("EEEEE");
      } else if (target == "short") {
        buffer.push_back('E');
      } else {  // target == "long"
        buffer.append("EEEE");
      }
    }
  }
  {
    // era
    const JSVal val = obj->GetField(JSDateTimeFormat::ERA);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "narrow") {
        buffer.append("GGGGG");
      } else if (target == "short") {
        buffer.push_back('G');
      } else {  // target == "long"
        buffer.append("GGGG");
      }
    }
  }
  {
    // year
    const JSVal val = obj->GetField(JSDateTimeFormat::YEAR);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("yy");
      } else {  // target == "numeric"
        buffer.append("yyyy");
      }
    }
  }
  {
    // month
    const JSVal val = obj->GetField(JSDateTimeFormat::MONTH);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("MM");
      } else if (target == "numeric") {
        buffer.push_back('M');
      } else if (target == "narrow") {
        buffer.append("MMMMM");
      } else if (target == "short") {
        buffer.append("MMM");
      } else {  // target == "long"
        buffer.append("MMMM");
      }
    }
  }
  {
    // day
    const JSVal val = obj->GetField(JSDateTimeFormat::DAY);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("dd");
      } else {  // target == "numeric"
        buffer.append("d");
      }
    }
  }
  {
    // hour
    // check hour12 (AM / PM style or 24 style)
    const JSVal val = obj->GetField(JSDateTimeFormat::HOUR);
    const JSVal hour12 = obj->GetField(JSDateTimeFormat::HOUR12);
    if (!val.IsUndefined()) {
      const bool two_digit = val.string()->GetUTF8() == "2-digit";
      if (JSVal::StrictEqual(hour12, JSTrue)) {
        buffer.append(two_digit ? "hh" : "h");
      } else {
        buffer.append(two_digit ? "HH" : "H");
      }
    }
  }
  {
    // minute
    const JSVal val = obj->GetField(JSDateTimeFormat::MINUTE);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("mm");
      } else {  // target == "numeric"
        buffer.append("m");
      }
    }
  }
  {
    // second
    const JSVal val = obj->GetField(JSDateTimeFormat::SECOND);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("ss");
      } else {  // target == "numeric"
        buffer.append("s");
      }
    }
  }
  {
    // timezone name
    const JSVal val = obj->GetField(JSDateTimeFormat::TIME_ZONE_NAME);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "short") {
        buffer.append("v");
      } else {  // target == "long"
        buffer.append("vvvv");
      }
    }
  }

  std::unique_ptr<icu::TimeZone> timezone;
  {
    // See http://userguide.icu-project.org/datetime/timezone
    const JSVal val = obj->GetField(JSDateTimeFormat::TIME_ZONE);
    if (val.IsUndefined()) {
      timezone.reset(icu::TimeZone::createDefault());
    } else {
      // already checked as UTC
      timezone.reset(icu::TimeZone::createTimeZone("GMT"));
    }
  }

  UErrorCode err = U_ZERO_ERROR;

  // See http://icu-project.org/apiref/icu4c/classCalendar.html
  // Calendar
  std::unique_ptr<icu::Calendar> calendar(
      icu::Calendar::createInstance(timezone.get(), locale, err));
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "calendar failed");
    return JSEmpty;
  }
  timezone.release();  // release

  const icu::UnicodeString skeleton(buffer.c_str());
  std::unique_ptr<icu::DateTimePatternGenerator>
      generator(icu::DateTimePatternGenerator::createInstance(locale, err));
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "pattern generator failed");
    return JSEmpty;
  }
  const icu::UnicodeString pattern = generator->getBestPattern(skeleton, err);
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "pattern generator failed");
    return JSEmpty;
  }
  icu::SimpleDateFormat* format =
      new icu::SimpleDateFormat(pattern, locale, err);
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "date formatter failed");
    return JSEmpty;
  }
  obj->set_date_time_format(format);

  format->adoptCalendar(calendar.get());
  JSString* str = JSString::NewAsciiString(ctx, calendar->getType(), IV_LV5_ERROR(e));
  obj->SetField(JSDateTimeFormat::CALENDAR, str);
  calendar.release();  // release
  return obj;
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSI18N_ICU_H_
