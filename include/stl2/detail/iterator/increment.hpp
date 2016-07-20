// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Casey Carter 2015
//  Copyright Eric Niebler 2015
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
#ifndef STL2_DETAIL_ITERATOR_INCREMENT_HPP
#define STL2_DETAIL_ITERATOR_INCREMENT_HPP

#include <cstddef>
#include <stl2/type_traits.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>
#include <stl2/detail/concepts/compare.hpp>
#include <stl2/detail/concepts/fundamental.hpp>

STL2_OPEN_NAMESPACE {
	///////////////////////////////////////////////////////////////////////////
	// difference_type_t [iterator.assoc]
	// Extension: defaults to make_unsigned_t<decltype(t - t)> when
	//     decltype(t - t) models Integral, in addition to doing so when T
	// itself models Integral.
	//
	// Not to spec:
	// * Strips cv-qualifiers before applying difference_type (see
	//   value_type_t for why)
	// * Requires difference_type_t to model SignedIntegral
	//
	namespace detail {
		template <class T>
		concept bool MemberDifferenceType =
			requires { typename T::difference_type; };
	}

	template <class> struct difference_type {};

	template <class T>
	struct difference_type<T*> {
		using type = std::ptrdiff_t;
	};
	template <_Is<is_void> T>
	struct difference_type<T*> {};

	template <detail::MemberDifferenceType T>
	struct difference_type<T> {
		using type = typename T::difference_type;
	};

	template <class T>
		requires !detail::MemberDifferenceType<T> &&
			_IsNot<T, is_pointer> &&
			requires (const T& a, const T& b) {
				STL2_DEDUCTION_CONSTRAINT(a - b, Integral);
			}
	struct difference_type<T> :
		make_signed<decltype(declval<const T&>() - declval<const T&>())> {};

	template <class T>
		requires SignedIntegral<meta::_t<difference_type<remove_cv_t<T>>>>()
	using difference_type_t =
		meta::_t<difference_type<remove_cv_t<T>>>;

	//namespace detail {
	//  template <class T, class...Us>
	//  concept bool OneOf() {
	//    //return Same<T, Us>() || ...;
	//    return meta::_v<meta::any_of<meta::list<Us...>, meta::bind_front<meta::quote<std::is_same>, T>>>;
	//  }
	//}

	///////////////////////////////////////////////////////////////////////////
	// WeaklyIncrementable [weaklyincrementable.iterators]
	//
	template <class I>
	concept bool WeaklyIncrementable() {
		return Semiregular<I>() &&
			requires (I& i) {
				typename difference_type_t<I>;
				STL2_EXACT_TYPE_CONSTRAINT(++i, I&);
				//STL2_BINARY_DEDUCTION_CONSTRAINT(++i, detail::OneOf, I&, const I&);
				i++;
			};
	}

	///////////////////////////////////////////////////////////////////////////
	// Incrementable [incrementable.iterators]
	//
	template <class I>
	concept bool Incrementable() {
		return WeaklyIncrementable<I>() &&
			EqualityComparable<I>() &&
			requires (I& i) {
				STL2_EXACT_TYPE_CONSTRAINT(i++, I);
				//STL2_BINARY_DEDUCTION_CONSTRAINT(i++, detail::OneOf, I, I&, const I&);
			};
	}

	namespace ext {
		///////////////////////////////////////////////////////////////////////////
		// Decrementable [Extension]
		//
		template <class I>
		concept bool Decrementable() {
			return Incrementable<I>() && requires (I& i) {
				STL2_EXACT_TYPE_CONSTRAINT(--i, I&);
				STL2_EXACT_TYPE_CONSTRAINT(i--, I);
			};
			// Let a and b be objects of type I.
			// Axiom: &--a == &a
			// Axiom: bool(a == b) implies bool(a-- == b)
			// Axiom: bool(a == b) implies bool((a--, a) == --b)
			// Axiom: bool(a == b) implies bool(--(++a) == b)
			// Axiom: bool(a == b) implies bool(++(--a) == b)
		}

		///////////////////////////////////////////////////////////////////////////
		// RandomAccessIncrementable [Extension]
		//
		template <class I>
		concept bool RandomAccessIncrementable() {
			return Decrementable<I>() &&
				requires (I& i, const I& ci, const difference_type_t<I> n) {
					STL2_EXACT_TYPE_CONSTRAINT(i += n, I&);
					STL2_EXACT_TYPE_CONSTRAINT(i -= n, I&);
					STL2_EXACT_TYPE_CONSTRAINT(ci + n, I);
					STL2_EXACT_TYPE_CONSTRAINT(n + ci, I);
					STL2_EXACT_TYPE_CONSTRAINT(ci - n, I);
					{ ci - ci } -> difference_type_t<I>;
				};
			// FIXME: Axioms
		}
	}
} STL2_CLOSE_NAMESPACE

#endif
