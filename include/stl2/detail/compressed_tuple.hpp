// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Casey Carter 2017
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
#ifndef STL2_DETAIL_COMPRESSED_TUPLE_HPP
#define STL2_DETAIL_COMPRESSED_TUPLE_HPP

#include <type_traits>
#include <utility>
#include <meta/meta.hpp>
#include <stl2/functional.hpp>
#include <stl2/detail/ebo_box.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/tagged.hpp>
#include <stl2/detail/concepts/object.hpp>

STL2_OPEN_NAMESPACE {
	namespace __compressed_tuple {
		// tagging individual elements with the complete type list disambiguates
		// base classes when composing compressed_tuples recursively.
		template <class T, std::size_t I, class... Ts>
		using storage = detail::ebo_box<T, meta::list<meta::size_t<I>, Ts...>>;

		template <class Types, class Indices> class ctuple;

		template <class... Ts, std::size_t... Is>
		requires
			(models::Destructible<Ts> && ...) &&
			models::Same<std::index_sequence<Is...>, std::index_sequence_for<Ts...>>
		class ctuple<meta::list<Ts...>, std::index_sequence<Is...>>
		: storage<Ts, Is, Ts...>...
		{
		public:
			ctuple() = default;

			// TODO: EXPLICIT
#if STL2_WORKAROUND_GCC_68812
			template <class... Args,
				meta::if_c<(models::Constructible<Ts, Args> && ...), int> = 42>
#else  // STL2_WORKAROUND_GCC_68812
			template <class... Args>
			requires (models::Constructible<Ts, Args> && ...)
#endif // STL2_WORKAROUND_GCC_68812
			constexpr ctuple(Args&&... args)
			noexcept((std::is_nothrow_constructible<Ts, Args>::value && ...))
			: storage<Ts, Is, Ts...>{std::forward<Args>(args)}...
			{}

#if STL2_WORKAROUND_GCC_68812
			template <class... Us,
				meta::if_c<(models::Constructible<Us, const Ts&> && ...), int> = 42>
#else  // STL2_WORKAROUND_GCC_68812
			template <class... Us>
			requires (models::Constructible<Us, const Ts&> && ...)
#endif // STL2_WORKAROUND_GCC_68812
			constexpr operator std::tuple<Us...> () const
			noexcept(noexcept(std::tuple<Us...>{std::declval<const Ts&>()...}))
			{
				return std::tuple<Us...>{get<Is>(*this)...};
			}

			template <std::size_t I, class T = meta::at_c<meta::list<Ts...>, I>>
			friend constexpr T& get(ctuple& ct) noexcept
			{
				return static_cast<storage<T, I, Ts...>&>(ct).get();
			}
			template <std::size_t I, class T = meta::at_c<meta::list<Ts...>, I>>
			friend constexpr const T& get(const ctuple& ct) noexcept
			{
				return static_cast<const storage<T, I, Ts...>&>(ct).get();
			}
			template <std::size_t I, class T = meta::at_c<meta::list<Ts...>, I>>
			friend constexpr T&& get(ctuple&& ct) noexcept
			{
				return static_cast<storage<T, I, Ts...>&&>(ct).get();
			}
			template <std::size_t I, class T = meta::at_c<meta::list<Ts...>, I>>
			friend constexpr const T&& get(const ctuple&& ct) noexcept
			{
				return static_cast<const storage<T, I, Ts...>&&>(ct).get();
			}
		};
	} // namespace __compressed_tuple

	namespace ext {
		template <class... Ts>
		using compressed_tuple =
			__compressed_tuple::ctuple<meta::list<Ts...>, std::index_sequence_for<Ts...>>;

		template <class... Args>
		requires (models::Constructible<__unwrap<Args>, Args> && ...)
		constexpr auto make_compressed_tuple(Args&&... args)
		STL2_NOEXCEPT_RETURN(
			compressed_tuple<__unwrap<Args>...>{std::forward<Args>(args)...}
		)

		template <class... Ts>
		using tagged_compressed_tuple =
			tagged<compressed_tuple<__tag_elem<Ts>...>, __tag_spec<Ts>...>;
	} // namespace ext
} STL2_CLOSE_NAMESPACE

namespace std {
	template <class... Ts, size_t... Is>
	struct tuple_size< ::__stl2::__compressed_tuple::ctuple<::meta::list<Ts...>, ::std::index_sequence<Is...>>>
	: integral_constant<size_t, sizeof...(Ts)>
	{};

	template <size_t I, class... Ts, size_t... Is>
	requires I < sizeof...(Ts)
	struct tuple_element<I, ::__stl2::__compressed_tuple::ctuple<::meta::list<Ts...>, ::std::index_sequence<Is...>>> {
		using type = ::meta::at_c<::meta::list<Ts...>, I>;
	};
} // namespace std

#endif
