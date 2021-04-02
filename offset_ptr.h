//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef OFFSET_PTR_H_
#define OFFSET_PTR_H_

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace impl {

struct static_cast_tag {};
struct dynamic_cast_tag {};
struct reinterpret_cast_tag {};
struct const_cast_tag {};

template <class T, class U>
struct is_cv_same {
  static const bool value =
      std::is_same<typename std::remove_cv<T>::type,
                   typename std::remove_cv<U>::type>::value;
};

template <class RawPointer>
class pointer_uintptr_caster;

template <class T>
class pointer_uintptr_caster<T*> {
 public:
  explicit pointer_uintptr_caster(uintptr_t sz) : m_uintptr(sz) {}

  explicit pointer_uintptr_caster(const volatile T* p)
      : m_uintptr(reinterpret_cast<uintptr_t>(p)) {}

  uintptr_t uintptr() const { return m_uintptr; }

  T* pointer() const { return reinterpret_cast<T*>(m_uintptr); }

 private:
  uintptr_t m_uintptr;
};
// Note: using the address of a local variable to point to another address
// is not standard conforming and this can be optimized-away by the compiler.
// Non-inlining is a method to remain illegal but correct

// Undef OFFSET_PTR_INLINE_XXX if your compiler can inline
// this code without breaking the library

////////////////////////////////////////////////////////////////////////
//
//                      offset_ptr_to_raw_pointer
//
////////////////////////////////////////////////////////////////////////
#define OFFSET_PTR_BRANCHLESS_TO_PTR
inline void* offset_ptr_to_raw_pointer(const volatile void* this_ptr,
                                       uintptr_t offset) {
  typedef pointer_uintptr_caster<void*> caster_t;
#ifndef OFFSET_PTR_BRANCHLESS_TO_PTR
  if (offset == 1) {
    return 0;
  } else {
    return caster_t(caster_t(this_ptr).uintptr() + offset).pointer();
  }
#else
  uintptr_t mask = offset == 1;
  --mask;
  uintptr_t target_offset = caster_t(this_ptr).uintptr() + offset;
  target_offset &= mask;
  return caster_t(target_offset).pointer();
#endif
}

////////////////////////////////////////////////////////////////////////
//
//                      offset_ptr_to_offset
//
////////////////////////////////////////////////////////////////////////
#define OFFSET_PTR_BRANCHLESS_TO_OFF
inline uintptr_t offset_ptr_to_offset(const volatile void* ptr,
                                      const volatile void* this_ptr) {
  typedef pointer_uintptr_caster<void*> caster_t;
#ifndef OFFSET_PTR_BRANCHLESS_TO_OFF
  // offset == 1 && ptr != 0 is not legal for this pointer
  if (!ptr) {
    return 1;
  } else {
    uintptr_t offset = caster_t(ptr).uintptr() - caster_t(this_ptr).uintptr();
    return offset;
  }
#else
  // const uintptr_t other = -uintptr_t(ptr != 0);
  // const uintptr_t offset = (caster_t(ptr).uintptr() -
  // caster_t(this_ptr).uintptr()) & other;
  // return offset + uintptr_t(!other);
  //
  uintptr_t offset = caster_t(ptr).uintptr() - caster_t(this_ptr).uintptr();
  --offset;
  uintptr_t mask = uintptr_t(ptr == 0);
  --mask;
  offset &= mask;
  return ++offset;
#endif
}

////////////////////////////////////////////////////////////////////////
//
//                      offset_ptr_to_offset_from_other
//
////////////////////////////////////////////////////////////////////////
#define OFFSET_PTR_BRANCHLESS_TO_OFF_FROM_OTHER
inline uintptr_t offset_ptr_to_offset_from_other(const volatile void* this_ptr,
                                                 const volatile void* other_ptr,
                                                 uintptr_t other_offset) {
  typedef pointer_uintptr_caster<void*> caster_t;
#ifndef OFFSET_PTR_BRANCHLESS_TO_OFF_FROM_OTHER
  if (other_offset == 1) {
    return 1;
  } else {
    uintptr_t offset = caster_t(other_ptr).uintptr() -
                       caster_t(this_ptr).uintptr() + other_offset;
    return offset;
  }
#else
  uintptr_t mask = other_offset == 1;
  --mask;
  uintptr_t offset =
      caster_t(other_ptr).uintptr() - caster_t(this_ptr).uintptr();
  offset &= mask;
  return offset + other_offset;
#endif
}

////////////////////////////////////////////////////////////////////////
//
// Let's assume cast to void and cv cast don't change any target address
//
////////////////////////////////////////////////////////////////////////
template <class From, class To>
struct offset_ptr_maintains_address {
  static const bool value = impl::is_cv_same<From, To>::value ||
                            impl::is_cv_same<void, To>::value ||
                            impl::is_cv_same<char, To>::value;
};

template <class From, class To, class Ret = void>
struct enable_if_convertible_equal_address
    : std::enable_if<std::is_convertible<From*, To*>::value &&
                         offset_ptr_maintains_address<From, To>::value,
                     Ret> {};

template <class From, class To, class Ret = void>
struct enable_if_convertible_unequal_address
    : std::enable_if<std::is_convertible<From*, To*>::value &&
                         !offset_ptr_maintains_address<From, To>::value,
                     Ret> {};

}  // namespace impl

//! A smart pointer that stores the offset between between the pointer and the
//! the object it points. This allows offset allows special properties, since
//! the pointer is independent from the address of the pointee, if the
//! pointer and the pointee are still separated by the same offset. This feature
//! converts offset_ptr in a smart pointer that can be placed in shared memory
//! and
//! memory mapped files mapped in different addresses in every process.
//!
//! \tparam PointedType The type of the pointee.
//! \tparam DifferenceType A signed integer type that can represent the
//! arithmetic operations on the pointer
//! \tparam OffsetType An unsigned integer type that can represent the
//!   distance between two pointers reinterpret_cast-ed as unsigned integers.
//!   This type
//!   should be at least of the same size of std::uintptr_t. In some systems
//!   it's possible to communicate
//!   between 32 and 64 bit processes using 64 bit offsets.
//!
//! Note: offset_ptr uses implementation defined properties, present in
//! most platforms, for
//! performance reasons:
//!   - Assumes that uintptr_t representation of nullptr is (uintptr_t)zero.
//!   - Assumes that incrementing a uintptr_t obtained from a pointer is
//!   equivalent
//!     to incrementing the pointer and then converting it back to uintptr_t.
template <class PointedType,
          class DifferenceType = std::ptrdiff_t,
          class OffsetType = uintptr_t>
class offset_ptr {
  // typedef offset_ptr<PointedType, DifferenceType, OffsetType> self_t;
  // void unspecified_bool_type_func() const {}
  // typedef void (self_t::*unspecified_bool_type)() const;

 public:
  typedef PointedType element_type;
  typedef PointedType* pointer;
  typedef typename std::add_lvalue_reference<PointedType>::type reference;
  typedef typename std::remove_volatile<
      typename std::remove_const<PointedType>::type>::type value_type;
  typedef DifferenceType difference_type;
  typedef std::random_access_iterator_tag iterator_category;
  typedef OffsetType offset_type;

 public:  // Public Functions
  //! Default constructor (null pointer).
  //! Never throws.
  offset_ptr() : m_offset(1) {}

  //! Constructor from raw pointer (allows "0" pointer conversion).
  //! Never throws.
  offset_ptr(pointer ptr)
      : m_offset(
            static_cast<OffsetType>(impl::offset_ptr_to_offset(ptr, this))) {}

  //! Constructor from other pointer.
  //! Never throws.
  template <class T>
  offset_ptr(T* ptr,
             typename std::enable_if<
                 std::is_convertible<T*, PointedType*>::value>::type* = 0)
      : m_offset(static_cast<OffsetType>(
            impl::offset_ptr_to_offset(static_cast<PointedType*>(ptr), this))) {
  }

  //! Constructor from other offset_ptr
  //! Never throws.
  offset_ptr(const offset_ptr& ptr)
      : m_offset(static_cast<OffsetType>(
            impl::offset_ptr_to_offset_from_other(this, &ptr, ptr.m_offset))) {}

  //! Constructor from other offset_ptr. If pointers of pointee types are
  //! convertible, offset_ptrs will be convertibles. Never throws.
  template <class T2>
  offset_ptr(
      const offset_ptr<T2, DifferenceType, OffsetType>& ptr,
      typename impl::enable_if_convertible_equal_address<T2,
                                                         PointedType>::type* =
          0)
      : m_offset(static_cast<OffsetType>(
            impl::offset_ptr_to_offset_from_other(this,
                                                  &ptr,
                                                  ptr.get_offset()))) {}

  //! Constructor from other offset_ptr. If pointers of pointee types are
  //! convertible, offset_ptrs will be convertibles. Never throws.
  template <class T2>
  offset_ptr(
      const offset_ptr<T2, DifferenceType, OffsetType>& ptr,
      typename impl::enable_if_convertible_unequal_address<T2,
                                                           PointedType>::type* =
          0)
      : m_offset(static_cast<OffsetType>(
            impl::offset_ptr_to_offset(static_cast<PointedType*>(ptr.get()),
                                       this))) {}

  //! Emulates static_cast operator.
  //! Never throws.
  template <class T2, class P2, class O2>
  offset_ptr(const offset_ptr<T2, P2, O2>& r, impl::static_cast_tag)
      : m_offset(static_cast<OffsetType>(
            impl::offset_ptr_to_offset(static_cast<PointedType*>(r.get()),
                                       this))) {}

  //! Emulates const_cast operator.
  //! Never throws.
  template <class T2, class P2, class O2>
  offset_ptr(const offset_ptr<T2, P2, O2>& r, impl::const_cast_tag)
      : m_offset(static_cast<OffsetType>(
            impl::offset_ptr_to_offset(const_cast<PointedType*>(r.get()),
                                       this))) {}

  //! Emulates dynamic_cast operator.
  //! Never throws.
  template <class T2, class P2, class O2>
  offset_ptr(const offset_ptr<T2, P2, O2>& r, impl::dynamic_cast_tag)
      : m_offset(static_cast<OffsetType>(
            impl::offset_ptr_to_offset(dynamic_cast<PointedType*>(r.get()),
                                       this))) {}

  //! Emulates reinterpret_cast operator.
  //! Never throws.
  template <class T2, class P2, class O2>
  offset_ptr(const offset_ptr<T2, P2, O2>& r, impl::reinterpret_cast_tag)
      : m_offset(static_cast<OffsetType>(
            impl::offset_ptr_to_offset(reinterpret_cast<PointedType*>(r.get()),
                                       this))) {}

  //! Obtains raw pointer from offset.
  //! Never throws.
  pointer get() const {
    return (pointer)impl::offset_ptr_to_raw_pointer(this, this->m_offset);
  }

  offset_type get_offset() const { return this->m_offset; }

  //! Pointer-like -> operator. It can return 0 pointer.
  //! Never throws.
  pointer operator->() const { return this->get(); }

  //! Dereferencing operator, if it is a null offset_ptr behavior
  //!   is undefined. Never throws.
  reference operator*() const {
    pointer p = this->get();
    reference r = *p;
    return r;
  }

  //! Indexing operator.
  //! Never throws.
  reference operator[](difference_type idx) const { return this->get()[idx]; }

  //! Assignment from pointer (saves extra conversion).
  //! Never throws.
  offset_ptr& operator=(pointer from) {
    this->m_offset =
        static_cast<OffsetType>(impl::offset_ptr_to_offset(from, this));
    return *this;
  }

  //! Assignment from other offset_ptr.
  //! Never throws.
  offset_ptr& operator=(const offset_ptr& ptr) {
    this->m_offset = static_cast<OffsetType>(
        impl::offset_ptr_to_offset_from_other(this, &ptr, ptr.m_offset));
    return *this;
  }

  //! Assignment from related offset_ptr. If pointers of pointee types
  //!   are assignable, offset_ptrs will be assignable. Never throws.
  template <class T2>
  typename std::enable_if<std::is_convertible<T2*, PointedType*>::value,
                          offset_ptr&>::type
  operator=(const offset_ptr<T2, DifferenceType, OffsetType>& ptr) {
    this->assign(
        ptr, std::integral_constant<bool, impl::offset_ptr_maintains_address<
                                              T2, PointedType>::value>());
    return *this;
  }

 public:
  //! offset_ptr += difference_type.
  //! Never throws.
  offset_ptr& operator+=(difference_type offset) {
    this->inc_offset(offset * sizeof(PointedType));
    return *this;
  }

  //! offset_ptr -= difference_type.
  //! Never throws.
  offset_ptr& operator-=(difference_type offset) {
    this->dec_offset(offset * sizeof(PointedType));
    return *this;
  }

  //! ++offset_ptr.
  //! Never throws.
  offset_ptr& operator++(void) {
    this->inc_offset(sizeof(PointedType));
    return *this;
  }

  //! offset_ptr++.
  //! Never throws.
  offset_ptr operator++(int) {
    offset_ptr tmp(*this);
    this->inc_offset(sizeof(PointedType));
    return tmp;
  }

  //! --offset_ptr.
  //! Never throws.
  offset_ptr& operator--(void) {
    this->dec_offset(sizeof(PointedType));
    return *this;
  }

  //! offset_ptr--.
  //! Never throws.
  offset_ptr operator--(int) {
    offset_ptr tmp(*this);
    this->dec_offset(sizeof(PointedType));
    return tmp;
  }

  //! safe bool conversion operator.
  //! Never throws.
  // operator unspecified_bool_type() const {
  //   return this->m_offset != 1 ? &self_t::unspecified_bool_type_func : 0;
  // }
  explicit operator bool() const { return this->m_offset != 1; }

  //! raw pointer conversion operator.
  //! Never throws
  operator pointer() const { return this->get(); }

  //! Not operator. Not needed in theory, but improves portability.
  //! Never throws
  bool operator!() const { return this->m_offset == 1; }

  //! Compatibility with pointer_traits
  //!
  template <class U>
  struct rebind {
    typedef offset_ptr<U, DifferenceType, OffsetType> other;
  };

  //! difference_type + offset_ptr
  //! operation
  friend offset_ptr operator+(difference_type diff, offset_ptr right) {
    right += diff;
    return right;
  }

  //! offset_ptr + difference_type
  //! operation
  friend offset_ptr operator+(offset_ptr left, difference_type diff) {
    left += diff;
    return left;
  }

  //! offset_ptr - diff
  //! operation
  friend offset_ptr operator-(offset_ptr left, difference_type diff) {
    left -= diff;
    return left;
  }

  //! offset_ptr - diff
  //! operation
  friend offset_ptr operator-(difference_type diff, offset_ptr right) {
    right -= diff;
    return right;
  }

  //! offset_ptr - offset_ptr
  //! operation
  friend difference_type operator-(const offset_ptr& pt,
                                   const offset_ptr& pt2) {
    return difference_type(pt.get() - pt2.get());
  }

  // Comparison
  friend bool operator==(const offset_ptr& pt1, const offset_ptr& pt2) {
    return pt1.get() == pt2.get();
  }

  friend bool operator!=(const offset_ptr& pt1, const offset_ptr& pt2) {
    return pt1.get() != pt2.get();
  }

  friend bool operator<(const offset_ptr& pt1, const offset_ptr& pt2) {
    return pt1.get() < pt2.get();
  }

  friend bool operator<=(const offset_ptr& pt1, const offset_ptr& pt2) {
    return pt1.get() <= pt2.get();
  }

  friend bool operator>(const offset_ptr& pt1, const offset_ptr& pt2) {
    return pt1.get() > pt2.get();
  }

  friend bool operator>=(const offset_ptr& pt1, const offset_ptr& pt2) {
    return pt1.get() >= pt2.get();
  }

  // Comparison to raw ptr to support literal 0
  friend bool operator==(pointer pt1, const offset_ptr& pt2) {
    return pt1 == pt2.get();
  }

  friend bool operator!=(pointer pt1, const offset_ptr& pt2) {
    return pt1 != pt2.get();
  }

  friend bool operator<(pointer pt1, const offset_ptr& pt2) {
    return pt1 < pt2.get();
  }

  friend bool operator<=(pointer pt1, const offset_ptr& pt2) {
    return pt1 <= pt2.get();
  }

  friend bool operator>(pointer pt1, const offset_ptr& pt2) {
    return pt1 > pt2.get();
  }

  friend bool operator>=(pointer pt1, const offset_ptr& pt2) {
    return pt1 >= pt2.get();
  }

  // Comparison
  friend bool operator==(const offset_ptr& pt1, pointer pt2) {
    return pt1.get() == pt2;
  }

  friend bool operator!=(const offset_ptr& pt1, pointer pt2) {
    return pt1.get() != pt2;
  }

  friend bool operator<(const offset_ptr& pt1, pointer pt2) {
    return pt1.get() < pt2;
  }

  friend bool operator<=(const offset_ptr& pt1, pointer pt2) {
    return pt1.get() <= pt2;
  }

  friend bool operator>(const offset_ptr& pt1, pointer pt2) {
    return pt1.get() > pt2;
  }

  friend bool operator>=(const offset_ptr& pt1, pointer pt2) {
    return pt1.get() >= pt2;
  }

  friend void swap(offset_ptr& left, offset_ptr& right) {
    pointer ptr = right.get();
    right = left;
    left = ptr;
  }

 private:
  template <class T2>
  void assign(const offset_ptr<T2, DifferenceType, OffsetType>& ptr,
              std::true_type) {  // no need to pointer adjustment
    this->m_offset = static_cast<OffsetType>(
        impl::offset_ptr_to_offset_from_other(this, &ptr, ptr.get_offset()));
  }

  template <class T2>
  void assign(const offset_ptr<T2, DifferenceType, OffsetType>& ptr,
              std::false_type) {  // we must convert to raw before calculating
                                  // the offset
    this->m_offset = static_cast<OffsetType>(
        impl::offset_ptr_to_offset(static_cast<PointedType*>(ptr.get()), this));
  }

  void inc_offset(DifferenceType bytes) { m_offset += bytes; }

  void dec_offset(DifferenceType bytes) { m_offset -= bytes; }

  OffsetType m_offset;  // Distance between this object and pointee address

 public:
  const OffsetType& priv_offset() const { return m_offset; }

  OffsetType& priv_offset() { return m_offset; }
};

//! operator<<
//! for offset ptr
template <class E, class T, class W, class X, class Y>
inline std::basic_ostream<E, T>& operator<<(std::basic_ostream<E, T>& os,
                                            offset_ptr<W, X, Y> const& p) {
  return os << p.get_offset();
}

//! operator>>
//! for offset ptr
template <class E, class T, class W, class X, class Y>
inline std::basic_istream<E, T>& operator>>(std::basic_istream<E, T>& is,
                                            offset_ptr<W, X, Y>& p) {
  return is >> p.get_offset();
}

//! Simulation of static_cast between pointers. Never throws.
template <class T1, class P1, class O1, class T2, class P2, class O2>
offset_ptr<T1, P1, O1> static_pointer_cast(const offset_ptr<T2, P2, O2>& r) {
  return offset_ptr<T1, P1, O1>(r, impl::static_cast_tag());
}

//! Simulation of const_cast between pointers. Never throws.
template <class T1, class P1, class O1, class T2, class P2, class O2>
offset_ptr<T1, P1, O1> const_pointer_cast(const offset_ptr<T2, P2, O2>& r) {
  return offset_ptr<T1, P1, O1>(r, impl::const_cast_tag());
}

//! Simulation of dynamic_cast between pointers. Never throws.
template <class T1, class P1, class O1, class T2, class P2, class O2>
offset_ptr<T1, P1, O1> dynamic_pointer_cast(const offset_ptr<T2, P2, O2>& r) {
  return offset_ptr<T1, P1, O1>(r, impl::dynamic_cast_tag());
}

//! Simulation of reinterpret_cast between pointers. Never throws.
template <class T1, class P1, class O1, class T2, class P2, class O2>
offset_ptr<T1, P1, O1> reinterpret_pointer_cast(
    const offset_ptr<T2, P2, O2>& r) {
  return offset_ptr<T1, P1, O1>(r, impl::reinterpret_cast_tag());
}

#endif  // OFFSET_PTR_H_
