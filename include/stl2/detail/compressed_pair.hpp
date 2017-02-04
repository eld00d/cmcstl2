// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Casey Carter 2015, 2017
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
#ifndef STL2_DETAIL_COMPRESSED_PAIR_HPP
#define STL2_DETAIL_COMPRESSED_PAIR_HPP

#include <utility>
#include <stl2/detail/compressed_tuple.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>
#include <stl2/detail/tagged.hpp>
#include <stl2/detail/concepts/object.hpp>

STL2_OPEN_NAMESPACE {
	namespace ext {
		template <class First, class Second>
		requires
			models::Destructible<First> &&
			models::Destructible<Second>
		struct compressed_pair
		: tagged_compressed_tuple<tag::first(First), tag::second(Second)>
		{
		private:
			using base_t = tagged_compressed_tuple<tag::first(First), tag::second(Second)>;
		public:
#if STL2_WORKAROUND_GCC_79143
			compressed_pair() = default;

			// TODO: EXPLICIT
			template <class Arg1, class Arg2>
			requires
				models::Constructible<First, Arg1> &&
				models::Constructible<Second, Arg2>
			constexpr compressed_pair(Arg1&& first, Arg2&& second)
			noexcept(std::is_nothrow_constructible<First, Arg1>::value &&
				std::is_nothrow_constructible<Second, Arg2>::value )
			: base_t(std::forward<Arg1>(first), std::forward<Arg2>(second))
			{}

#else  // STL2_WORKAROUND_GCC_79143
			using base_t::base_t;
#endif // STL2_WORKAROUND_GCC_79143

			using base_t::first;
			using base_t::second;

			template <class F, class S>
			requires
				models::Constructible<F, const First&> &&
				models::Constructible<S, const Second&>
			constexpr operator std::pair<F, S> () const {
				return std::pair<F, S>{first(), second()};
			}
		};

		template <class First, class Second>
		requires
			models::Constructible<__unwrap<First>, First> &&
			models::Constructible<__unwrap<Second>, Second>
		constexpr auto make_compressed_pair(First&& f, Second&& s)
		STL2_NOEXCEPT_RETURN(
			compressed_pair<__unwrap<First>, __unwrap<Second>>{
				std::forward<First>(f), std::forward<Second>(s)
			}
		)
	} // namespace ext
} STL2_CLOSE_NAMESPACE

namespace std {
	template <class First, class Second>
	struct tuple_size< ::__stl2::ext::compressed_pair<First, Second>>
	: integral_constant<size_t, 2>
	{};

	template <class First, class Second>
	struct tuple_element<0, ::__stl2::ext::compressed_pair<First, Second>> {
		using type = First;
	};

	template <class First, class Second>
	struct tuple_element<1, ::__stl2::ext::compressed_pair<First, Second>> {
		using type = Second;
	};
} // namespace std

#endif
