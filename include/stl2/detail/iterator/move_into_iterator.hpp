// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Casey Carter 2015
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
#ifndef STL2_DETAIL_ITERATOR_MOVE_INTO_ITERATOR_HPP
#define STL2_DETAIL_ITERATOR_MOVE_INTO_ITERATOR_HPP

#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>
#include <stl2/detail/proxy_move.hpp>
#include <stl2/detail/concepts/compare.hpp>
#include <stl2/detail/concepts/core.hpp>
#include <stl2/detail/iterator/concepts.hpp>

STL2_OPEN_NAMESPACE {
  struct __mii_access {
    template <class MII>
    constexpr auto&& current(MII&& mii) noexcept {
      return __stl2::forward<MII>(mii).current_;
    }
  };

  Iterator{O}
  class move_into_iterator {
    friend __mii_access;
    O current_{};
  public:
    using difference_type = difference_type_t<O>;

    move_into_iterator() = default;
    explicit STL2_CONSTEXPR_EXT move_into_iterator(O o)
    noexcept(is_nothrow_move_constructible<O>::value)
    : current_(__stl2::move(o))
    {}
    template <ConvertibleTo<O> U>
    STL2_CONSTEXPR_EXT move_into_iterator(move_into_iterator<U> u)
    noexcept(is_nothrow_constructible<O, U&&>::value)
    : current_(__mii_access::current(__stl2::move(u)))
    {}

    template <ConvertibleTo<O> U>
    STL2_CONSTEXPR_EXT move_into_iterator& operator=(move_into_iterator<U> u) &
    noexcept(is_nothrow_constructible<O, U&&>::value)
    {
      current_ = __mii_access::current(__stl2::move(u));
      return *this;
    }

    STL2_CONSTEXPR_EXT O base() const
    noexcept(is_nothrow_copy_constructible<O>::value)
    {
      return current_;
    }

    STL2_CONSTEXPR_EXT move_into_iterator& operator++() &
    noexcept(noexcept(++current_))
    {
      ++current_;
      return *this;
    }
    STL2_CONSTEXPR_EXT move_into_iterator operator++(int) &
    noexcept(is_nothrow_copy_constructible<O>::value &&
             is_nothrow_move_constructible<O>::value &&
             noexcept(++current_))
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    STL2_CONSTEXPR_EXT move_into_iterator& operator*() noexcept {
      return *this;
    }

    template <class T>
    requires
      !models::Same<decay_t<T>, move_into_iterator> &&
      models::Writable<O, decltype(__stl2::proxy_move(__stl2::declval<T>()))>
    STL2_CONSTEXPR_EXT move_into_iterator& operator=(T&& t)
    noexcept(noexcept(
      *current_ = __stl2::proxy_move(__stl2::forward<T>(t))))
    {
      *current_ = __stl2::proxy_move(__stl2::forward<T>(t));
      return *this;
    }
  };

  EqualityComparable{O1, O2}
  STL2_CONSTEXPR_EXT bool operator==(
    const move_into_iterator<O1>& x, const move_into_iterator<O2>& y)
  STL2_NOEXCEPT_RETURN(
    static_cast<bool>(__mii_access::current(x) == __mii_access::current(y))
  )

  EqualityComparable{O1, O2}
  STL2_CONSTEXPR_EXT bool operator!=(
    const move_into_iterator<O1>& x, const move_into_iterator<O2>& y)
  STL2_NOEXCEPT_RETURN(
    !(x == y)
  )

  template <class O>
  requires
    models::Iterator<__f<O>>
  STL2_CONSTEXPR_EXT auto move_into(O&& o)
  STL2_NOEXCEPT_RETURN(
    move_into_iterator<__f<O>>(__stl2::forward<O>(o))
  )
} STL2_CLOSE_NAMESPACE

#endif
