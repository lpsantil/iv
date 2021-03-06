#ifndef IV_MAYBE_H_
#define IV_MAYBE_H_
#include <cassert>
#include <algorithm>
#include <type_traits>

// Maybe nullptr pointer wrapper class
// not have ownership

namespace iv {
namespace core {

template<class T>
class Maybe {
 public:
  typedef Maybe<T> this_type;
  typedef void (this_type::*bool_type)() const;

  Maybe(T* ptr) : ptr_(ptr) { }  // NOLINT
  Maybe() : ptr_(nullptr) { }

  template<class U>
  Maybe(const Maybe<U>& rhs,
        typename std::enable_if<std::is_convertible<U*, T*>::value>::type* = 0)
    : ptr_(rhs.get_address_maybe_null()) {
  }

  T& operator*() const {
    assert(ptr_);
    return *ptr_;
  }

  T* Address() const {
    assert(ptr_);
    return ptr_;
  }

  operator bool_type() const {
    return ptr_ != nullptr ?
        &this_type::this_type_does_not_support_comparisons : 0;
  }

  template<typename U>
  this_type& operator=(const Maybe<U>& rhs) {
    if (this != &rhs) {
      this_type(rhs).swap(*this);
    }
    return *this;
  }

  inline void swap(this_type& rhs) {
    using std::swap;
    swap(ptr_, rhs.ptr_);
  }

  inline friend void swap(this_type& lhs, this_type& rhs) {
    return lhs.swap(rhs);
  }

  inline T* get_address_maybe_null() const {
    return ptr_;
  }

 private:
  void this_type_does_not_support_comparisons() const { }

  T* ptr_;
};

} }  // namespace iv::core
#endif  // IV_MAYBE_H_
