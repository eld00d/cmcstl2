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
#ifndef STL2_DETAIL_RANGE_CONCEPTS_HPP
#define STL2_DETAIL_RANGE_CONCEPTS_HPP

#include <initializer_list>

#include <stl2/type_traits.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>
#include <stl2/detail/concepts/compare.hpp>
#include <stl2/detail/concepts/core.hpp>
#include <stl2/detail/concepts/object.hpp>
#include <stl2/detail/iterator/concepts.hpp>
#include <stl2/detail/range/access.hpp>

namespace std {
#ifndef __GLIBCXX__
#pragma message "These forward declarations will likely only work with libstdc++."
#endif
	template<class, class, class> class set;
	template<class, class, class> class multiset;
	template<class, class, class, class> class unordered_set;
	template<class, class, class, class> class unordered_multiset;
}

STL2_OPEN_NAMESPACE {
	///////////////////////////////////////////////////////////////////////////
	// Range [ranges.range]
	//
	template <class T>
	using iterator_t = decltype(__stl2::begin(declval<T&>()));

	template <class T>
	using sentinel_t = decltype(__stl2::end(declval<T&>()));

	template <class T>
	concept bool Range() {
		return requires { typename sentinel_t<T>; };
	}

	///////////////////////////////////////////////////////////////////////////
	// SizedRange [ranges.sized]
	//
	template <class R>
	constexpr bool disable_sized_range = false;

	template <class R>
	concept bool SizedRange() {
		return Range<R>() &&
			!disable_sized_range<__uncvref<R>> &&
			requires (const remove_reference_t<R>& r) {
				STL2_DEDUCTION_CONSTRAINT(__stl2::size(r), Integral);
				STL2_CONVERSION_CONSTRAINT(__stl2::size(r), difference_type_t<iterator_t<R>>);
			};
	}

	///////////////////////////////////////////////////////////////////////////
	// View [ranges.view]
	//
	struct view_base {};

	template <class T>
	concept bool _ContainerLike() {
		return Range<T>() && Range<const T>() &&
			!Same<reference_t<iterator_t<T>>, reference_t<iterator_t<const T>>>();
	}

	template <class T>
	struct enable_view {};

	template <class T>
	constexpr bool __view_predicate = true;

	template <class T>
		requires _Valid<meta::_t, enable_view<T>>
	constexpr bool __view_predicate<T> = meta::_v<enable_view<T>>;

	// TODO: Be very certain that "!" here works as intended.
	template <_ContainerLike T>
		requires !(DerivedFrom<T, view_base>() ||
			_Valid<meta::_t, enable_view<T>>)
	constexpr bool __view_predicate<T> = false;

	template <class T>
	constexpr bool __view_predicate<std::initializer_list<T>> = false;
	template <class Key, class Compare, class Alloc>
	constexpr bool __view_predicate<std::set<Key, Compare, Alloc>> = false;
	template <class Key, class Compare, class Alloc>
	constexpr bool __view_predicate<std::multiset<Key, Compare, Alloc>> = false;
	template <class Key, class Hash, class Pred, class Alloc>
	constexpr bool __view_predicate<std::unordered_set<Key, Hash, Pred, Alloc>> = false;
	template <class Key, class Hash, class Pred, class Alloc>
	constexpr bool __view_predicate<std::unordered_multiset<Key, Hash, Pred, Alloc>> = false;

	template <class T>
	concept bool View() {
		return Range<T>() &&
			__view_predicate<T> &&
			Semiregular<T>();
	}

	///////////////////////////////////////////////////////////////////////////
	// BoundedRange [ranges.bounded]
	//
	template <class T>
	concept bool BoundedRange() {
		return Range<T>() && Same<iterator_t<T>, sentinel_t<T>>();
	}

	///////////////////////////////////////////////////////////////////////////
	// OutputRange [ranges.output]
	// Not to spec: value category sensitive.
	//
	template <class R, class T>
	concept bool OutputRange() {
		return Range<R>() && OutputIterator<iterator_t<R>, T>();
	}

	///////////////////////////////////////////////////////////////////////////
	// InputRange [ranges.input]
	//
	template <class T>
	concept bool InputRange() {
		return Range<T>() && InputIterator<iterator_t<T>>();
	}

	///////////////////////////////////////////////////////////////////////////
	// ForwardRange [ranges.forward]
	//
	template <class T>
	concept bool ForwardRange() {
		return Range<T>() && ForwardIterator<iterator_t<T>>();
	}

	///////////////////////////////////////////////////////////////////////////
	// BidirectionalRange [ranges.bidirectional]
	//
	template <class T>
	concept bool BidirectionalRange() {
		return Range<T>() && BidirectionalIterator<iterator_t<T>>();
	}

	///////////////////////////////////////////////////////////////////////////
	// RandomAccessRange [ranges.random.access]
	//
	template <class T>
	concept bool RandomAccessRange() {
		return Range<T>() && RandomAccessIterator<iterator_t<T>>();
	}

	namespace ext {
		template <class R>
		concept bool ContiguousRange() {
			return SizedRange<R>() && ext::ContiguousIterator<iterator_t<R>>() &&
				requires (remove_reference_t<R>& r) {
					STL2_EXACT_TYPE_CONSTRAINT(__stl2::data(r), add_pointer_t<reference_t<iterator_t<R>>>);
				};
		}
	}
} STL2_CLOSE_NAMESPACE

#endif
