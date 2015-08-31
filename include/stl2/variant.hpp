#ifndef STL2_VARIANT_HPP
#define STL2_VARIANT_HPP

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <meta/meta.hpp>
#include <stl2/functional.hpp>
#include <stl2/tuple.hpp>
#include <stl2/type_traits.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/tagged.hpp>
#include <stl2/detail/tuple_like.hpp>
#include <stl2/detail/concepts/compare.hpp>
#include <stl2/detail/concepts/fold_expressions.hpp>
#include <stl2/detail/concepts/object.hpp>
#include <stl2/detail/variant/storage.hpp>

#define __cpp_lib_variant 20150524

#if 1
// Disable asserts in the visitation machinery that ICE the compiler.
// (Probably https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66635)
#define STL2_VISIT_ASSERT(...)
#else
#define STL2_VISIT_ASSERT assert
#endif

namespace stl2 { inline namespace v1 {
struct monostate {};

constexpr bool operator==(monostate, monostate)
{ return true; }
constexpr bool operator!=(monostate, monostate)
{ return false; }
constexpr bool operator<(monostate, monostate)
{ return false; }
constexpr bool operator>(monostate, monostate)
{ return false; }
constexpr bool operator<=(monostate, monostate)
{ return true; }
constexpr bool operator>=(monostate, monostate)
{ return true; }

namespace __variant {
struct void_storage : monostate {
  void_storage() requires false;
private:
  // void_storage must have at least one constexpr constructor
  // to be a literal type.
  struct hack_tag {};
  constexpr void_storage(hack_tag) {}
};

template <class T>
struct element_storage_ {
  using type = T;
};
template <_Is<is_reference> T>
struct element_storage_<T> {
  using type = reference_wrapper<remove_reference_t<T>>;
};
template <>
struct element_storage_<void> {
  using type = void_storage;
};
template <class T>
using element_t = meta::_t<element_storage_<T>>;

template <class T, class Types,
    std::size_t I = meta::_v<meta::find_index<Types, T>>>
  requires I != meta::_v<meta::npos> &&
    Same<meta::find_index<meta::drop_c<Types, I + 1>, T>, meta::npos>()
constexpr std::size_t index_of_type = I;

template <class...Ts>
  requires detail::AllDestructible<element_t<Ts>...>
class base;

template <class...Types>
meta::list<Types...> types_(base<Types...>& b); // not defined

template <class T>
using VariantTypes =
  decltype(types_(declval<__uncvref<T>&>()));

// satisfied iff __uncvref<T> is derived from a
// specialization of __variant::base
template <class T>
concept bool Variant =
  requires { typename VariantTypes<T>; };
} // namespace __variant

template <class...Ts>
  requires detail::AllDestructible<__variant::element_t<Ts>...>
class variant;

template <class>
struct emplaced_type_t {};

namespace {
template <class T>
constexpr auto& emplaced_type =
  detail::static_const<emplaced_type_t<T>>::value;
}

template <std::size_t I>
using emplaced_index_t = meta::size_t<I>;

namespace {
template <std::size_t I>
constexpr auto& emplaced_index =
  detail::static_const<emplaced_index_t<I>>::value;
}

class bad_variant_access : public std::logic_error {
public:
  using std::logic_error::logic_error;
  bad_variant_access() :
    std::logic_error{"Attempt to access invalid variant alternative"} {}
};

template <class T, __variant::Variant V,
  std::size_t I = __variant::index_of_type<T, __variant::VariantTypes<V>>>
constexpr bool holds_alternative(const V& v) noexcept {
  return v.index() == I;
}

namespace __variant {
struct destruct_fn {
  Destructible{T}
  void operator()(T& t) const noexcept {
    t.~T();
  }

  template <Destructible T, std::size_t N>
  void operator()(T (&a)[N]) const noexcept {
    std::size_t i = N;
    while (i > 0) {
      a[--i].~T();
    }
  }
};
namespace {
  constexpr auto& destruct = detail::static_const<destruct_fn>::value;
}

struct construct_fn {
  Constructible{T, ...Args}
  void operator()(T& t, Args&&...args) const
    noexcept(is_nothrow_constructible<T, Args...>::value) {
    ::new(static_cast<void*>(std::addressof(t)))
      T{std::forward<Args>(args)...};
  }
};
namespace {
  constexpr auto& construct = detail::static_const<construct_fn>::value;
}

struct copy_move_tag {};

template <_IsNot<is_const> T>
constexpr remove_reference_t<T>&
cook(element_t<meta::id_t<T>>& t) noexcept {
  return t;
}

template <class T>
constexpr const remove_reference_t<T>&
cook(const element_t<meta::id_t<T>>& t) noexcept {
  return t;
}

template <class T>
constexpr T&& cook(element_t<meta::id_t<T>>&& t) noexcept {
  return stl2::move(cook<T>(t));
}

using non_void_predicate =
  meta::compose<meta::quote<meta::not_>, meta::quote_trait<is_void>>;

template <class Types>
using non_void_types = meta::filter<Types, non_void_predicate>;

// Convert a list of types into a sequence of the indices of the non-is_void types.
template <class Types>
using non_void_indices =
  meta::transform<
    meta::filter<
      meta::zip<meta::list<
        meta::as_list<meta::make_index_sequence<Types::size()>>,
        Types>>,
      meta::compose<non_void_predicate, meta::quote<meta::second>>>,
    meta::quote<meta::first>>;

template <class>
struct as_integer_sequence_ {};
template <class T, T...Is>
struct as_integer_sequence_<meta::list<std::integral_constant<T, Is>...>> {
  using type = std::integer_sequence<T, Is...>;
};
template <class T>
using as_integer_sequence = meta::_t<as_integer_sequence_<T>>;

template <class T>
constexpr auto& strip_cv(T& t) noexcept {
  return const_cast<remove_cv_t<T>&>(t);
}

template <class From, class To>
concept bool ViableAlternative =
  Same<decay_t<From>, decay_t<To>>() &&
  Constructible<To, From>();

template <class T, std::size_t I, class...Types>
struct constructible_from_ : false_type {};

template <class T, std::size_t I, class First, class...Rest>
struct constructible_from_<T, I, First, Rest...> :
  constructible_from_<T, I + 1, Rest...> {};

template <class T, std::size_t I, class First, class...Rest>
  requires ViableAlternative<T, First>
struct constructible_from_<T, I, First, Rest...> : true_type {
  static constexpr bool ambiguous = meta::_v<constructible_from_<T, I + 1, Rest...>>;
  static constexpr std::size_t index = I;
};

template <class T, class...Types>
using constructible_from =
  constructible_from_<T, 0u, Types...>;

template <class...Ts>
  requires detail::AllDestructible<element_t<Ts>...>
class base {
  friend struct v_access;
protected:
  using storage_t = storage<element_t<Ts>...>;
  using index_t =
#if 1
    std::size_t;
#else
    meta::if_c<
      (sizeof...(Ts) <= std::numeric_limits<std::int_fast8_t>::max()),
      std::int_fast8_t,
      meta::if_c<
        (sizeof...(Ts) <= std::numeric_limits<std::int_fast16_t>::max()),
        std::int_fast16_t,
        meta::if_c<
          (sizeof...(Ts) <= std::numeric_limits<std::int_fast32_t>::max()),
          std::int_fast32_t,
          meta::if_c<
            (sizeof...(Ts) <= std::numeric_limits<std::int_fast64_t>::max()),
            std::int_fast64_t,
            std::size_t>>>>;
#endif
  static_assert(sizeof...(Ts) - is_unsigned<index_t>::value <
                std::numeric_limits<index_t>::max());
  static constexpr auto invalid_index = index_t(-1);

  storage_t storage_;
  index_t index_;

protected:
  void clear() noexcept {
    if (valid()) {
      raw_visit(destruct, *this);
      index_ = invalid_index;
    }
  }

  void clear() noexcept
    requires ext::TriviallyDestructible<storage_t>() {
    index_ = invalid_index;
  }

  template <class That>
    requires DerivedFrom<__uncvref<That>, base>()
  void copy_move_from(That&& that) {
    assert(!valid());
    if (that.valid()) {
      raw_visit_with_index([this](auto i, auto&& from) {
        construct(strip_cv(st_access::raw_get(i, storage_)),
                  stl2::forward<decltype(from)>(from));
        index_ = i;
      }, stl2::forward<That>(that));
    }
  }

  void move_assign_from(DerivedFrom<base>&& that) {
    if (index_ == that.index_) {
      if (valid()) {
        raw_visit_with_index([this](auto i, auto&& from) {
          st_access::raw_get(i, storage_) = stl2::forward<decltype(from)>(from);
        }, stl2::move(that));
      }
    } else {
      clear();
      copy_move_from(stl2::move(that));
    }
  }

  void copy_assign_from(const DerivedFrom<base>& that) {
    if (index_ == that.index_) {
      if (valid()) {
        raw_visit_with_index([this](auto i, const auto& from) {
          st_access::raw_get(i, storage_) = from;
        }, that);
      }
    } else {
      auto tmp = that;
      clear();
      copy_move_from(stl2::move(tmp));
    }
  }

  template <class That>
    requires DerivedFrom<__uncvref<That>, base>()
  base(copy_move_tag, That&& that) :
    storage_{empty_tag{}}, index_{invalid_index} {
    copy_move_from(stl2::forward<That>(that));
  }

public:
  using types = meta::list<Ts...>;

  constexpr base()
    noexcept(is_nothrow_default_constructible<storage_t>::value)
    requires DefaultConstructible<storage_t>() :
    index_{0} {}

  template <class T>
    requires !Same<decay_t<T>, base>() &&
      constructible_from<T&&, Ts...>::value &&
      !constructible_from<T&&, Ts...>::ambiguous &&
      Constructible<storage_t, emplaced_index_t<constructible_from<T&&, Ts...>::index>, T&&>()
  constexpr base(T&& t)
    noexcept(is_nothrow_constructible<storage_t, emplaced_index_t<constructible_from<T&&, Ts...>::index>, T&&>::value)
    : storage_{emplaced_index<constructible_from<T&&, Ts...>::index>, stl2::forward<T>(t)},
      index_{constructible_from<T&&, Ts...>::index} {}

  template <class T>
    requires !Same<decay_t<T>, base>() &&
      constructible_from<T&&, Ts...>::value &&
      !constructible_from<T&&, Ts...>::ambiguous
  constexpr base(T&& t)
    noexcept(is_nothrow_constructible<storage_t, emplaced_index_t<constructible_from<T&&, Ts...>::index>, T&>::value)
    : storage_{emplaced_index<constructible_from<T&&, Ts...>::index>, t},
      index_{constructible_from<T&&, Ts...>::index} {}

  template <class T>
    requires !Same<decay_t<T>, base>() &&
      constructible_from<T&&, Ts...>::value &&
      constructible_from<T&&, Ts...>::ambiguous
  base(T&&) = delete; // Conversion from T is ambiguous.

  template <std::size_t I, class...Args, _IsNot<is_reference> T = meta::at_c<types, I>>
    requires Constructible<T, Args...>()
  explicit constexpr base(emplaced_index_t<I>, Args&&...args)
    noexcept(is_nothrow_constructible<storage_t, emplaced_index_t<I>, Args...>::value)
    : storage_{emplaced_index<I>, stl2::forward<Args>(args)...}, index_{I} {}

  template <std::size_t I, class...Args, _Is<is_reference> T = meta::at_c<types, I>>
  explicit constexpr base(emplaced_index_t<I>, meta::id_t<T> t)
    noexcept(is_nothrow_constructible<storage_t, emplaced_index_t<I>, T&>::value)
    : storage_{emplaced_index<I>, t}, index_{I} {}

  template <_IsNot<is_reference> T, class...Args, std::size_t I = index_of_type<T, types>>
    requires Constructible<T, Args...>()
  explicit constexpr base(emplaced_type_t<T>, Args&&...args)
    noexcept(is_nothrow_constructible<storage_t, emplaced_index_t<I>, Args...>::value)
    : storage_{emplaced_index<I>, stl2::forward<Args>(args)...}, index_{I} {}

  template <_Is<is_reference> T, std::size_t I = index_of_type<T, types>>
  explicit constexpr base(emplaced_type_t<T>, meta::id_t<T> t)
    noexcept(is_nothrow_constructible<storage_t, emplaced_index_t<I>, T&>::value)
    : storage_{emplaced_index<I>, t}, index_{I} {}

  template <std::size_t I, class E, class...Args, _IsNot<is_reference> T = meta::at_c<types, I>>
    requires Constructible<T, Args...>()
  explicit constexpr base(emplaced_index_t<I>, std::initializer_list<E> il, Args&&...args)
    noexcept(is_nothrow_constructible<storage_t, emplaced_index_t<I>, std::initializer_list<E>, Args...>::value)
    : storage_{emplaced_index<I>, il, stl2::forward<Args>(args)...}, index_{I} {}

  template <_IsNot<is_reference> T, class E, class...Args, std::size_t I = index_of_type<T, types>>
    requires Constructible<T, Args...>()
  explicit constexpr base(emplaced_type_t<T>, std::initializer_list<E> il, Args&&...args)
    noexcept(is_nothrow_constructible<storage_t, emplaced_index_t<I>, std::initializer_list<E>, Args...>::value)
    : storage_{emplaced_index<I>, il, stl2::forward<Args>(args)...}, index_{I} {}

  template <_IsNot<is_reference> T, class...Args, std::size_t I = index_of_type<T, types>>
    requires Constructible<T, Args...>()
  void emplace(Args&&...args)
    noexcept(is_nothrow_constructible<element_t<T>, Args...>::value) {
    clear();
    construct(strip_cv(st_access::raw_get(meta::size_t<I>{}, storage_)),
              stl2::forward<Args>(args)...);
    index_ = I;
  }

  template <_Is<is_reference> T, std::size_t I = index_of_type<T, types>>
  void emplace(meta::id_t<T> t)
    noexcept(is_nothrow_constructible<element_t<T>, T&>::value) {
    clear();
    construct(st_access::raw_get(meta::size_t<I>{}, storage_), t);
    index_ = I;
  }

  template <_IsNot<is_reference> T, class E, class...Args, std::size_t I = index_of_type<T, types>>
    requires Constructible<T, std::initializer_list<E>, Args...>()
  void emplace(std::initializer_list<E> il, Args&&...args)
    noexcept(is_nothrow_constructible<element_t<T>, std::initializer_list<E>, Args...>::value) {
    clear();
    construct(strip_cv(st_access::raw_get(meta::size_t<I>{}, storage_)),
              il, stl2::forward<Args>(args)...);
    index_ = I;
  }

  template <std::size_t I, class...Args, _IsNot<is_reference> T = meta::at_c<types, I>>
    requires Constructible<T, Args...>()
  void emplace(Args&&...args)
    noexcept(is_nothrow_constructible<element_t<T>, Args...>::value) {
    clear();
    construct(strip_cv(st_access::raw_get(meta::size_t<I>{}, storage_)),
              stl2::forward<Args>(args)...);
    index_ = I;
  }

  template <std::size_t I, class...Args, _Is<is_reference> T = meta::at_c<types, I>>
  void emplace(meta::id_t<T> t)
    noexcept(is_nothrow_constructible<element_t<T>, T&>::value) {
    clear();
    construct(st_access::raw_get(meta::size_t<I>{}, storage_), t);
    index_ = I;
  }

  template <std::size_t I, class E, class...Args, _IsNot<is_reference> T = meta::at_c<types, I>>
    requires Constructible<T, std::initializer_list<E>, Args...>()
  void emplace(std::initializer_list<E> il, Args&&...args)
    noexcept(is_nothrow_constructible<element_t<T>, std::initializer_list<E>, Args...>::value) {
    clear();
    construct(strip_cv(st_access::raw_get(meta::size_t<I>{}, storage_)),
              il, stl2::forward<Args>(args)...);
    index_ = I;
  }

  constexpr std::size_t index() const noexcept {
    return index_;
  }
  constexpr bool valid() const noexcept
    requires is_unsigned<index_t>::value {
     // Consider index_ < invalid_index
    return index_ != invalid_index;
  }
  constexpr bool valid() const noexcept
    requires is_signed<index_t>::value {
    return index_ >= 0;
  }

  template <std::size_t I, Variant V>
    requires I < VariantTypes<V>::size() &&
      _IsNot<meta::at_c<VariantTypes<V>, I>, is_void>
  friend constexpr auto&& get(V&& v);
};

struct v_access {
  template <std::size_t I, Variant V,
    _IsNot<is_void> T = meta::at_c<VariantTypes<V>, I>>
  static constexpr auto&& raw_get(meta::size_t<I> i, V&& v) noexcept {
    STL2_VISIT_ASSERT(I == v.index());
    return st_access::raw_get(i, stl2::forward<V>(v).storage_);
  }

  template <std::size_t I, Variant V,
    _IsNot<is_void> T = meta::at_c<VariantTypes<V>, I>>
  static constexpr auto&& cooked_get(meta::size_t<I> i, V&& v) noexcept {
    assert(I == v.index());
    return cook<T>(v_access::raw_get(i, stl2::forward<V>(v)));
  }
};

// "inline" is here for the ODR; we do not actually
// want bad_access to be inlined into get. Having it
// be a separate function results in better generated
// code.
[[noreturn]] inline bool bad_access() {
  throw bad_variant_access{};
}

template <std::size_t I, Variant V>
  requires I < VariantTypes<V>::size() &&
    _IsNot<meta::at_c<VariantTypes<V>, I>, is_void>
constexpr auto&& get_unchecked(V&& v) {
  assert(v.index() == I);
  return v_access::cooked_get(meta::size_t<I>{}, stl2::forward<V>(v));
}

template <std::size_t I, Variant V>
  requires I < VariantTypes<V>::size() &&
    _IsNot<meta::at_c<VariantTypes<V>, I>, is_void>
constexpr auto&& get(V&& v) {
  assert(v.valid());
  // Odd syntax here to avoid
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67371
  v.index() == I || bad_access();
  return v_access::cooked_get(meta::size_t<I>{}, stl2::forward<V>(v));
}

template <_IsNot<is_void> T, Variant V,
  std::size_t I = index_of_type<T, VariantTypes<V>>>
constexpr auto&& get_unchecked(V&& v) {
  return __variant::get_unchecked<I>(v);
}

template <_IsNot<is_void> T, Variant V,
  std::size_t I = index_of_type<T, VariantTypes<V>>>
constexpr auto&& get(V&& v) {
  return __variant::get<I>(v);
}

template <std::size_t I, Variant V>
  requires I < VariantTypes<V>::size() &&
    _IsNot<meta::at_c<VariantTypes<V>, I>, is_void>
constexpr auto get(V* v) noexcept ->
  decltype(&__variant::v_access::cooked_get(meta::size_t<I>{}, *v)) {
  assert(v);
  if (v->index() == I) {
    return &__variant::v_access::cooked_get(meta::size_t<I>{}, *v);
  }
  return nullptr;
}

template <_IsNot<is_void> T, Variant V,
  std::size_t I = index_of_type<T, VariantTypes<V>>>
constexpr auto get(V* v) noexcept {
  return __variant::get<I>(v);
}

// Visitation
template <Variant...Variants>
constexpr std::size_t total_alternatives = 1;
template <Variant First, Variant...Rest>
constexpr std::size_t total_alternatives<First, Rest...> =
  non_void_types<VariantTypes<First>>::size() *
  total_alternatives<Rest...>;

template <class F, class Variants, class Indices>
struct single_visit_properties {};
template <class F, Variant...Variants, std::size_t...Is>
struct single_visit_properties<F,
  meta::list<Variants...>, meta::list<meta::size_t<Is>...>>
{
  using type =
    decltype(declval<F>()(std::index_sequence<Is...>{},
      v_access::raw_get(meta::size_t<Is>{}, declval<Variants>())...));
  static constexpr bool nothrow =
    noexcept(declval<F>()(std::index_sequence<Is...>{},
      v_access::raw_get(meta::size_t<Is>{}, declval<Variants>())...));
};
template <class F, class Variants, class Indices>
using single_visit_return_t =
  meta::_t<single_visit_properties<F, Variants, Indices>>;
template <class F, class Variants, class Indices>
using single_visit_noexcept =
  meta::bool_<single_visit_properties<F, Variants, Indices>::nothrow>;

template <Variant...Vs>
using all_visit_vectors =
  meta::cartesian_product<meta::list<non_void_indices<VariantTypes<Vs>>...>>;

template <class F, Variant...Vs>
using all_return_types =
  meta::transform<
    all_visit_vectors<Vs...>,
    meta::bind_front<meta::quote<single_visit_return_t>, F, meta::list<Vs...>>>;

#if 0
// require visitation to return the same type for all alternatives.
template <class F, Variant...Vs>
  requires meta::_v<meta::all_of<
    meta::pop_front<all_return_types<F, Vs...>>,
    meta::bind_front<
      meta::quote_trait<is_same>,
      meta::front<all_return_types<F, Vs...>>>>>
using VisitReturn = meta::front<all_return_types<F, Vs...>>;

#elif 0
// require the return type of all alternatives to have a common
// type which visit returns.
template <class F, Variant...Vs>
using VisitReturn =
  meta::apply_list<meta::quote<common_type_t>, all_return_types<F, Vs...>>;

#else
// require the return type of all alternatives to have a common
// reference type which visit returns.
template <class F, Variant...Vs>
using VisitReturn =
  meta::apply_list<meta::quote<common_reference_t>, all_return_types<F, Vs...>>;
#endif

template <class F, Variant...Vs>
concept bool RawVisitorWithIndices =
  sizeof...(Vs) > 0 &&
  requires { typename VisitReturn<F, Vs...>; };

RawVisitorWithIndices{F, ...Vs}
constexpr bool VisitNothrow =
  meta::_v<meta::apply_list<meta::quote<meta::fast_and>,
    meta::transform<all_visit_vectors<Vs...>,
      meta::bind_front<meta::quote<single_visit_noexcept>,
        F, meta::list<Vs...>>>>>;

// TODO: Tune.
constexpr std::size_t O1_visit_threshold = 5;

template <std::size_t...Is, Variant...Vs, RawVisitorWithIndices<Vs...> F>
constexpr VisitReturn<F, Vs...>
visit_handler(std::index_sequence<Is...> indices, F&& f, Vs&&...vs)
STL2_NOEXCEPT_RETURN(
  stl2::forward<F>(f)(indices,
    v_access::raw_get(meta::size_t<Is>{}, stl2::forward<Vs>(vs))...)
)

RawVisitorWithIndices{F, ...Vs}
class ON_dispatch {
  using R = VisitReturn<F, Vs...>;
  static constexpr std::size_t N = sizeof...(Vs);

  F&& f_;
  tuple<Vs&&...> vs_;

  template <std::size_t...Is, std::size_t...Js>
    requires sizeof...(Is) == N && sizeof...(Js) == N
  constexpr R invoke(std::index_sequence<Is...> i, std::index_sequence<Js...>)
    noexcept(noexcept(visit_handler(i, stl2::forward<F>(f_),
                                    stl2::get<Js>(stl2::move(vs_))...)))
  {
    return visit_handler(i, stl2::forward<F>(f_),
                         stl2::get<Js>(stl2::move(vs_))...);
  }

  template <std::size_t...Is>
    requires sizeof...(Is) == N
  constexpr R find_indices(std::index_sequence<Is...> i)
    noexcept(noexcept(declval<ON_dispatch&>().
      invoke(i, std::make_index_sequence<N>{}))) {
    return invoke(i, std::make_index_sequence<N>{});
  }

  template <std::size_t...Is, std::size_t Last>
  constexpr R find_one_index(std::index_sequence<Is...>, std::size_t n, std::index_sequence<Last>)
    noexcept(noexcept(declval<ON_dispatch&>().
      find_indices(std::index_sequence<Is..., Last>{}))) {
    assert(n == Last);
    (void)n;
    return find_indices(std::index_sequence<Is..., Last>{});
  }

  template <std::size_t...Is, std::size_t First, std::size_t...Rest>
  constexpr R find_one_index(std::index_sequence<Is...> i, std::size_t n, std::index_sequence<First, Rest...>)
    noexcept(noexcept(
      declval<ON_dispatch&>().find_indices(std::index_sequence<Is..., First>{}),
      declval<ON_dispatch&>().find_one_index(i, n, std::index_sequence<Rest...>{})
    ))
  {
    assert(n >= First);
    if (n <= First) {
      return find_indices(std::index_sequence<Is..., First>{});
    } else {
      return find_one_index(i, n, std::index_sequence<Rest...>{});
    }
  }

  template <std::size_t...Is,
    class NVI = as_integer_sequence<non_void_indices<
      VariantTypes<meta::at_c<meta::list<Vs...>, sizeof...(Is)>>>>>
    requires sizeof...(Is) < N
  constexpr R find_indices(std::index_sequence<Is...> i)
    noexcept(noexcept(declval<ON_dispatch&>()
      .find_one_index(i, std::size_t{0}, NVI{}))) {
    auto& v = stl2::get<sizeof...(Is)>(vs_);
    STL2_VISIT_ASSERT(v.valid());
    return find_one_index(i, v.index(), NVI{});
  }

public:
  constexpr ON_dispatch(F&& f, Vs&&...vs) noexcept :
    f_(stl2::forward<F>(f)), vs_{stl2::forward<Vs>(vs)...} {}

  constexpr R visit() &&
    noexcept(VisitNothrow<F, Vs...>) {
    return find_indices(std::index_sequence<>{});
  }
};

RawVisitorWithIndices{F, ...Vs}
constexpr VisitReturn<F, Vs...>
raw_visit_with_indices(F&& f, Vs&&...vs)
  noexcept(VisitNothrow<F, Vs...>)
  requires total_alternatives<Vs...> < O1_visit_threshold
{
  return ON_dispatch<F, Vs...>{
    stl2::forward<F>(f), stl2::forward<Vs>(vs)...
  }.visit();
}

template <class I, class F, Variant...Vs>
  requires RawVisitorWithIndices<F, Vs...>
constexpr VisitReturn<F, Vs...>
o1_visit_handler(F&& f, Vs&&...vs)
STL2_NOEXCEPT_RETURN(
  visit_handler(I{}, stl2::forward<F>(f),
    stl2::forward<Vs>(vs)...)
)

RawVisitorWithIndices{F, ...Vs}
using visit_handler_ptr =
  VisitReturn<F, Vs...>(*)(F&&, Vs&&...);

template <class Indices, class F, Variant...Vs>
  requires RawVisitorWithIndices<F, Vs...>
constexpr visit_handler_ptr<F, Vs...> visit_handler_for = {};
template <std::size_t...Is, class F, Variant...Vs>
  requires RawVisitorWithIndices<F, Vs...> &&
    meta::_v<meta::all_of<
      meta::list<meta::at_c<VariantTypes<Vs>, Is>...>,
      non_void_predicate>>
constexpr visit_handler_ptr<F, Vs...>
  visit_handler_for<std::index_sequence<Is...>, F, Vs...> =
    &o1_visit_handler<std::index_sequence<Is...>, F, Vs...>;

template <class Indices, class F, Variant...Vs>
  requires RawVisitorWithIndices<F, Vs...>
struct O1_dispatch;
template <class...Is, class F, Variant...Vs>
  requires RawVisitorWithIndices<F, Vs...>
struct O1_dispatch<meta::list<Is...>, F, Vs...> {
  static constexpr visit_handler_ptr<F, Vs...> table[] = {
    visit_handler_for<Is, F, Vs...>...
  };
};

template <class...Is, class F, Variant...Vs>
  requires RawVisitorWithIndices<F, Vs...>
constexpr visit_handler_ptr<F, Vs...>
  O1_dispatch<meta::list<Is...>, F, Vs...>::table[];

constexpr std::size_t calc_index() noexcept {
  return 0;
}

template <Variant First, Variant...Rest>
constexpr std::size_t
calc_index(const First& f, const Rest&...rest) noexcept {
  STL2_VISIT_ASSERT(f.valid());
  constexpr std::size_t M = meta::_v<meta::fold<
    meta::list<meta::size<VariantTypes<Rest>>...>,
    meta::size_t<1>,
    meta::quote<meta::multiplies>>>;
  return f.index() * M + calc_index(rest...);
}

template <Variant...Vs>
using all_index_vectors =
  meta::transform<
    meta::cartesian_product<meta::list<
      meta::as_list<meta::make_index_sequence<
        VariantTypes<Vs>::size()>>...>>,
    meta::quote<as_integer_sequence>>;

RawVisitorWithIndices{F, ...Vs}
constexpr VisitReturn<F, Vs...>
raw_visit_with_indices(F&& f, Vs&&...vs)
  noexcept(VisitNothrow<F, Vs...>)
  requires total_alternatives<Vs...> >= O1_visit_threshold
{
  using Dispatch = O1_dispatch<all_index_vectors<Vs...>, F, Vs...>;
  std::size_t i = calc_index(vs...);
  STL2_VISIT_ASSERT(Dispatch::table[i]);
  return Dispatch::table[i](stl2::forward<F>(f), stl2::forward<Vs>(vs)...);
}

template <class F>
struct single_index_visitor {
  F&& f_;

  template <std::size_t I, class T>
  constexpr decltype(auto) operator()(std::index_sequence<I>, T&& t) && {
    return stl2::forward<F>(f_)(meta::size_t<I>{}, stl2::forward<T>(t));
  }
};

template <class F, class V>
concept bool RawVisitorWithIndex =
  RawVisitorWithIndices<single_index_visitor<F>, V>;

RawVisitorWithIndex{F, V}
constexpr VisitReturn<single_index_visitor<F>, V>
raw_visit_with_index(F&& f, V&& v)
  noexcept(VisitNothrow<single_index_visitor<F>, V>) {
  return raw_visit_with_indices(
    single_index_visitor<F>{stl2::forward<F>(f)},
    stl2::forward<V>(v));
}

template <class F, Variant...Vs>
struct no_index_visitor {
  F&& f_;

  template <class I, class...Args>
  constexpr decltype(auto) operator()(I, Args&&...args) && {
    return stl2::forward<F>(f_)(stl2::forward<Args>(args)...);
  }
};

template <class F, class...Vs>
concept bool RawVisitor =
  RawVisitorWithIndices<no_index_visitor<F>, Vs...>;

RawVisitor{F, ...Vs}
constexpr VisitReturn<no_index_visitor<F>, Vs...>
raw_visit(F&& f, Vs&&...vs)
  noexcept(VisitNothrow<no_index_visitor<F>, Vs...>) {
  return raw_visit_with_indices(
    no_index_visitor<F>{stl2::forward<F>(f)},
    stl2::forward<Vs>(vs)...);
}

template <class F, Variant...Vs>
struct cooked_visitor {
  F&& f_;

  template <std::size_t...Is, class...Args>
  constexpr decltype(auto) operator()(std::index_sequence<Is...> i, Args&&...args) &&
    noexcept(noexcept(
      stl2::forward<F>(f_)(i,
        cook<meta::at_c<VariantTypes<Vs>, Is>>(
          stl2::forward<Args>(args))...))) {
    return stl2::forward<F>(f_)(i,
      cook<meta::at_c<VariantTypes<Vs>, Is>>(
        stl2::forward<Args>(args))...);
  }
};

template <class F, class...Vs>
concept bool VisitorWithIndices =
  RawVisitorWithIndices<cooked_visitor<F, Vs...>, Vs...>;

VisitorWithIndices{F, ...Vs}
constexpr VisitReturn<cooked_visitor<F, Vs...>, Vs...>
visit_with_indices(F&& f, Vs&&...vs)
  noexcept(VisitNothrow<cooked_visitor<F, Vs...>, Vs...>) {
  return raw_visit_with_indices(
    cooked_visitor<F, Vs...>{stl2::forward<F>(f)},
    stl2::forward<Vs>(vs)...);
}

template <class F, class V>
concept bool VisitorWithIndex =
  VisitorWithIndices<single_index_visitor<F>, V>;

VisitorWithIndex{F, V}
constexpr VisitReturn<cooked_visitor<single_index_visitor<F>, V>, V>
visit_with_index(F&& f, V&& v)
  noexcept(VisitNothrow<cooked_visitor<single_index_visitor<F>, V>, V>) {
  return visit_with_indices(
    single_index_visitor<F>{stl2::forward<F>(f)},
    stl2::forward<V>(v));
}

template <class F, class...Vs>
concept bool Visitor =
  VisitorWithIndices<no_index_visitor<F>, Vs...>;

Visitor{F, ...Vs}
constexpr VisitReturn<cooked_visitor<no_index_visitor<F>, Vs...>, Vs...>
visit(F&& f, Vs&&...vs)
  noexcept(VisitNothrow<cooked_visitor<no_index_visitor<F>, Vs...>, Vs...>) {
  return visit_with_indices(
    no_index_visitor<F>{stl2::forward<F>(f)},
    stl2::forward<Vs>(vs)...);
}

template <class...Ts>
class destruct_base : public base<Ts...> {
  using base_t = base<Ts...>;
public:
  ~destruct_base() noexcept {
    this->clear();
  }

  destruct_base() = default;
  destruct_base(destruct_base&&) = default;
  destruct_base(const destruct_base&) = default;
  destruct_base& operator=(destruct_base&&) & = default;
  destruct_base& operator=(const destruct_base&) & = default;

  using base_t::base_t;
};

template <class...Ts>
  requires ext::TriviallyDestructible<storage<element_t<Ts>...>>()
class destruct_base<Ts...> : public base<Ts...> {
  using base_t = base<Ts...>;
public:
  using base_t::base_t;
};

template <class...Ts>
class move_base : public destruct_base<Ts...> {
  using base_t = destruct_base<Ts...>;
public:
  move_base() = default;
  move_base(move_base&&) = delete;
  move_base(const move_base&) = default;
  move_base& operator=(move_base&&) & = default;
  move_base& operator=(const move_base&) & = default;

  using base_t::base_t;
};

template <class...Ts>
  requires detail::AllMoveConstructible<element_t<Ts>...>
class move_base<Ts...> : public destruct_base<Ts...> {
  using base_t = destruct_base<Ts...>;
public:
  move_base() = default;
  move_base(move_base&& that)
    noexcept(meta::_v<meta::all_of<meta::list<element_t<Ts>...>,
               meta::quote_trait<is_nothrow_move_constructible>>>) :
    base_t{copy_move_tag{}, stl2::move(that)} {}
  move_base(const move_base&) = default;
  move_base& operator=(move_base&&) & = default;
  move_base& operator=(const move_base&) & = default;

  using base_t::base_t;
};

template <class...Ts>
  requires detail::AllMoveConstructible<element_t<Ts>...> &&
    ext::TriviallyMoveConstructible<storage<element_t<Ts>...>>()
class move_base<Ts...> : public destruct_base<Ts...> {
  using base_t = destruct_base<Ts...>;
public:
  using base_t::base_t;
};

template <class...Ts>
class move_assign_base : public move_base<Ts...> {
  using base_t = move_base<Ts...>;
public:
  move_assign_base() = default;
  move_assign_base(move_assign_base&&) = default;
  move_assign_base(const move_assign_base&) = default;
  move_assign_base& operator=(move_assign_base&&) & = delete;
  move_assign_base& operator=(const move_assign_base&) & = default;

  using base_t::base_t;
};

template <class...Ts>
  requires detail::AllMovable<element_t<Ts>...>
class move_assign_base<Ts...> : public move_base<Ts...> {
  using base_t = move_base<Ts...>;
public:
  move_assign_base() = default;
  move_assign_base(move_assign_base&&) = default;
  move_assign_base(const move_assign_base&) = default;
  move_assign_base& operator=(move_assign_base&& that) &
    noexcept(meta::_v<meta::fast_and<
      meta::all_of<meta::list<element_t<Ts>...>,
        meta::quote_trait<is_nothrow_move_assignable>>,
      meta::all_of<meta::list<element_t<Ts>...>,
        meta::quote_trait<is_nothrow_move_constructible>>>>) {
    this->move_assign_from(stl2::move(that));
    return *this;
  }
  move_assign_base& operator=(const move_assign_base&) & = default;

  using base_t::base_t;
};

template <class...Ts>
  requires detail::AllMovable<element_t<Ts>...> &&
    ext::TriviallyMovable<storage<element_t<Ts>...>>()
class move_assign_base<Ts...> : public move_base<Ts...> {
  using base_t = move_base<Ts...>;
public:
  using base_t::base_t;
};

template <class...Ts>
class copy_base : public move_assign_base<Ts...> {
  using base_t = move_assign_base<Ts...>;
public:
  copy_base() = default;
  copy_base(copy_base&&) = default;
  copy_base(const copy_base&) = delete;
  copy_base& operator=(copy_base&&) & = default;
  copy_base& operator=(const copy_base&) & = default;

  using base_t::base_t;
};

template <class...Ts>
  requires detail::AllCopyConstructible<element_t<Ts>...>
class copy_base<Ts...> : public move_assign_base<Ts...> {
  using base_t = move_assign_base<Ts...>;
public:
  copy_base() = default;
  copy_base(copy_base&&) = default;
  copy_base(const copy_base& that)
    noexcept(meta::_v<meta::all_of<meta::list<element_t<Ts>...>,
               meta::quote_trait<is_nothrow_copy_constructible>>>) :
    base_t{copy_move_tag{}, that} {}
  copy_base& operator=(copy_base&&) & = default;
  copy_base& operator=(const copy_base&) & = default;

  using base_t::base_t;
};

template <class...Ts>
  requires detail::AllCopyConstructible<element_t<Ts>...> &&
    ext::TriviallyCopyConstructible<storage<element_t<Ts>...>>()
class copy_base<Ts...> : public move_assign_base<Ts...> {
  using base_t = move_assign_base<Ts...>;
public:
  using base_t::base_t;
};

template <class...Ts>
class copy_assign_base : public copy_base<Ts...> {
  using base_t = copy_base<Ts...>;
public:
  copy_assign_base() = default;
  copy_assign_base(copy_assign_base&&) = default;
  copy_assign_base(const copy_assign_base&) = default;
  copy_assign_base& operator=(copy_assign_base&&) & = default;
  copy_assign_base& operator=(const copy_assign_base&) & = delete;

  using base_t::base_t;
};

template <class...Ts>
  requires detail::AllCopyable<element_t<Ts>...>
class copy_assign_base<Ts...> : public copy_base<Ts...> {
  using base_t = copy_base<Ts...>;
public:
  copy_assign_base() = default;
  copy_assign_base(copy_assign_base&&) = default;
  copy_assign_base(const copy_assign_base&) = default;
  copy_assign_base& operator=(copy_assign_base&&) & = default;
  copy_assign_base& operator=(const copy_assign_base& that) &
    noexcept(meta::_v<meta::fast_and<
      meta::all_of<meta::list<element_t<Ts>...>,
        meta::quote_trait<is_nothrow_copy_assignable>>,
      meta::all_of<meta::list<element_t<Ts>...>,
        meta::quote_trait<is_nothrow_copy_constructible>>>>) {
    this->copy_assign_from(that);
    return *this;
  }

  using base_t::base_t;
};

template <class...Ts>
  requires detail::AllCopyable<element_t<Ts>...> &&
    ext::TriviallyCopyable<storage<element_t<Ts>...>>()
class copy_assign_base<Ts...> : public copy_base<Ts...> {
  using base_t = copy_base<Ts...>;
public:
  using base_t::base_t;
};
} // namespace __variant

template <class...Ts>
  requires detail::AllDestructible<__variant::element_t<Ts>...>
class variant : public __variant::copy_assign_base<Ts...> {
  using base_t = __variant::copy_assign_base<Ts...>;

  template <class T>
  using constructible_from = __variant::constructible_from<T, Ts...>;

  struct swap_visitor {
    variant& self_;

    constexpr void operator()(auto i, auto& from)
      noexcept(is_nothrow_swappable_v<decltype((from)), decltype((from))>) {
      stl2::swap(__variant::v_access::raw_get(i, self_), from);
    }
  };

  struct equal_to_visitor {
    const variant& self_;

    constexpr bool operator()(auto i, const auto& o) const
      noexcept(noexcept(o == o)) {
      const auto& s = __variant::v_access::cooked_get(i, self_);
      static_assert(is_same<decltype(o), decltype(s)>());
      return s == o;
    }
  };

  struct less_than_visitor {
    const variant& self_;

    constexpr bool operator()(auto i, const auto& o) const
      noexcept(noexcept(o < o)) {
      const auto& s = __variant::v_access::cooked_get(i, self_);
      static_assert(is_same<decltype(o), decltype(s)>());
      return s < o;
    }
  };

public:
  using types = meta::list<Ts...>;
  using base_t::base_t;

  template <_IsNot<is_reference> T, class CF = constructible_from<T&&>>
    requires CF::value && !CF::ambiguous &&
      Same<T, meta::at_c<types, CF::index>>() &&
      Movable<T>()
  variant& operator=(T&& t) &
    noexcept(is_nothrow_move_constructible<T>::value &&
             is_nothrow_move_assignable<T>::value) {
    constexpr auto I = CF::index;
    if (this->index_ == I) {
      auto& target = __variant::v_access::raw_get(meta::size_t<I>{}, *this);
      target = stl2::move(t);
    } else {
      this->template emplace<T>(stl2::move(t));
    }
    return *this;
  }

  template <_IsNot<is_reference> T, class CF = constructible_from<const T&>>
    requires CF::value && !CF::ambiguous &&
      Same<T, meta::at_c<types, CF::index>>() &&
      Copyable<T>()
  variant& operator=(const T& t) &
    noexcept(is_nothrow_copy_constructible<T>::value &&
             is_nothrow_copy_assignable<T>::value &&
             is_nothrow_move_constructible<T>::value) {
    constexpr auto I = CF::index;
    if (this->index_ == I) {
      auto& target = __variant::v_access::raw_get(meta::size_t<I>{}, *this);
      target = t;
    } else {
      this->template emplace<T>(T{t});
    }
    return *this;
  }

  template <class T, class CF = constructible_from<T&&>>
    requires CF::value && CF::ambiguous
  variant& operator=(T&&) & = delete; // Assignment from T is ambiguous.

  template <class T, class CF = constructible_from<T&&>>
    requires CF::value && !CF::ambiguous &&
      _Is<meta::at_c<types, CF::index>, is_reference>
  variant& operator=(T&&) & = delete; // Assignment to reference alternatives is ill-formed.

  constexpr void swap(variant& that)
    noexcept(is_nothrow_move_constructible<variant>::value &&
             is_nothrow_move_assignable<variant>::value &&
             meta::_v<meta::and_c<is_nothrow_swappable_v<
               __variant::element_t<Ts>&, __variant::element_t<Ts>&>...>>)
    requires Movable<base_t>() && // FIXME: Movable<variant>()
      detail::AllSwappable<__variant::element_t<Ts>&...>
  {
    if (this->index_ == that.index_) {
      if (this->valid()) {
        __variant::raw_visit_with_index(swap_visitor{*this}, that);
      }
    } else {
      stl2::swap(static_cast<base_t&>(*this),
                 static_cast<base_t&>(that));
    }
  }

  friend constexpr void swap(variant& lhs, variant& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
    requires requires { lhs.swap(rhs); }
  {
    lhs.swap(rhs);
  }

  friend constexpr bool operator==(const variant& lhs, const variant& rhs)
    requires detail::AllEqualityComparable<__variant::element_t<Ts>...>
  {
    if (lhs.index_ != rhs.index_) {
      return false;
    }
    return __variant::visit_with_index(equal_to_visitor{lhs}, rhs);
  }

  friend constexpr bool operator!=(const variant& lhs, const variant& rhs)
    requires detail::AllEqualityComparable<__variant::element_t<Ts>...>
  {
    return !(lhs == rhs);
  }

  friend constexpr bool operator<(const variant& lhs, const variant& rhs)
    requires detail::AllTotallyOrdered<__variant::element_t<Ts>...>
  {
    if (lhs.index_ < rhs.index_) {
      return true;
    } else if (lhs.index_ == rhs.index_) {
      return __variant::visit_with_index(less_than_visitor{lhs}, rhs);
    } else {
      return false;
    }
  }

  friend constexpr bool operator>(const variant& lhs, const variant& rhs)
    requires detail::AllTotallyOrdered<__variant::element_t<Ts>...>
  {
    return rhs < lhs;
  }

  friend constexpr bool operator<=(const variant& lhs, const variant& rhs)
    requires detail::AllTotallyOrdered<__variant::element_t<Ts>...>
  {
    return !(rhs < lhs);
  }

  friend constexpr bool operator>=(const variant& lhs, const variant& rhs)
    requires detail::AllTotallyOrdered<__variant::element_t<Ts>...>
  {
    return !(lhs < rhs);
  }
};

template <>
class variant<> {
  variant() requires false;
};

using __variant::get;
using __variant::get_unchecked;
using __variant::visit;
using __variant::visit_with_index;
using __variant::visit_with_indices;

template <TaggedType...Ts>
using tagged_variant =
  tagged<variant<__tag_elem<Ts>...>, __tag_spec<Ts>...>;

template <class T, __variant::Variant V>
constexpr std::size_t tuple_find<T, V> =
  __variant::index_of_type<T, __variant::VariantTypes<V>>;

namespace __variant {
// Lifted from Boost.
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template <class T>
concept bool Hashable =
  requires (const T& e) {
    typename std::hash<T>;
    std::hash<T>{}(e);
  };

template <class...>
constexpr bool AllHashable = false;
template <Hashable...Ts>
constexpr bool AllHashable<Ts...> = true;
}}} // namespace stl2::v1::detail

#undef STL2_CONSTEXPR_VISIT

namespace std {
template <::stl2::__variant::Variant V>
struct tuple_size<V> :
  ::meta::size<::stl2::__variant::VariantTypes<V>> {};

template <size_t I, ::stl2::__variant::Variant V>
struct tuple_element<I, V> :
  ::meta::at_c<::stl2::__variant::VariantTypes<V>, I> {};

template <>
struct hash<::stl2::monostate> {
  using result_type = size_t;
  using argument_type = ::stl2::monostate;

  constexpr size_t operator()(::stl2::monostate) const {
    // https://xkcd.com/221/
    return 4;
  }
};

template <>
struct hash<::stl2::__variant::void_storage> {
  using result_type = size_t;
  using argument_type = ::stl2::__variant::void_storage;

  [[noreturn]] size_t
  operator()(const ::stl2::__variant::void_storage&) const {
    std::terminate();
  }
};

template <class...Ts>
  requires ::stl2::__variant::AllHashable<::stl2::__variant::element_t<Ts>...>
struct hash<::stl2::variant<Ts...>> {
private:
  struct hash_visitor {
    std::size_t seed = 0u;

    constexpr void operator()(const auto& e) {
      ::stl2::__variant::hash_combine(seed, e);
    }
  };

public:
  using result_type = size_t;
  using argument_type = ::stl2::variant<Ts...>;

  constexpr size_t operator()(const argument_type& v) const {
    hash_visitor visitor;
    visitor(v.index());
    ::stl2::__variant::raw_visit(visitor, v);
    return visitor.seed;
  }
};
}

#endif
