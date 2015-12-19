#ifndef STL2_DETAIL_PROXY_MOVE_HPP
#define STL2_DETAIL_PROXY_MOVE_HPP

#include <utility>
#include <vector>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>
#include <stl2/detail/concepts/core.hpp>

STL2_OPEN_NAMESPACE {
  namespace __proxy_move {
    template <class T>
    T&& proxy_move(std::reference_wrapper<T>&& rw)
    STL2_NOEXCEPT_RETURN(
      __stl2::move(rw.get())
    )

    bool proxy_move(std::vector<bool>::reference&& vbr)
    noexcept(noexcept(static_cast<bool>(vbr)))
    requires is_class<std::vector<bool>::reference>::value
    {
      return static_cast<bool>(vbr);
    }

    bool proxy_move(std::vector<bool>::const_reference&& vbr)
    noexcept(noexcept(static_cast<bool>(vbr)))
    requires is_class<std::vector<bool>::const_reference>::value
    {
      return static_cast<bool>(vbr);
    }

    template <class T>
    using proxy_move_field =
      meta::if_<is_reference<T>, remove_reference_t<T>&&, T>;

    template <class First, class Second>
    auto proxy_move(std::pair<First, Second>&& p)
    STL2_NOEXCEPT_RETURN(
      std::pair<proxy_move_field<First>, proxy_move_field<Second>>{
        __stl2::move(p.first), __stl2::move(p.second)
      }
    )

    template <std::size_t...Is, class...Ts>
    auto proxy_move_(std::index_sequence<Is...>, std::tuple<Ts...>&& t)
    STL2_NOEXCEPT_RETURN(
      std::tuple<proxy_move_field<Ts>...>{
        std::get<Is>(__stl2::move(t))...
      }
    )

    template <class...Ts>
    decltype(auto) proxy_move(std::tuple<Ts...>&& t)
    STL2_NOEXCEPT_RETURN(
      proxy_move_(std::index_sequence_for<Ts...>{}, __stl2::move(t))
    )

    template <class>
    constexpr bool has_customization = false;
    template <class T>
    requires
      requires(T&& t) {
        STL2_DEDUCE_AUTO_REF_REF(proxy_move((T&&)t));
      }
    constexpr bool has_customization<T> = true;

    struct fn {
      template <_IsNot<is_reference> T>
      requires has_customization<T>
      constexpr decltype(auto) operator()(T&& t) const
      STL2_NOEXCEPT_RETURN(
        proxy_move((T&&)t)
      )

      template <class T>
      constexpr decltype(auto) operator()(T&& t) const
      STL2_NOEXCEPT_RETURN(
        __stl2::move((T&&)t)
      )
    };
  }
  inline namespace {
    constexpr auto& proxy_move =
      detail::static_const<__proxy_move::fn>::value;
  }

  template <class T>
  constexpr bool __is_proxy_reference =
    is_lvalue_reference<T>::value ||
    (!is_reference<T>::value && __proxy_move::has_customization<T>);
} STL2_CLOSE_NAMESPACE

#endif
