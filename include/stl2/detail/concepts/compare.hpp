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
#ifndef STL2_DETAIL_CONCEPTS_COMPARE_HPP
#define STL2_DETAIL_CONCEPTS_COMPARE_HPP

#include <stl2/type_traits.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>
#include <stl2/detail/concepts/core.hpp>
#include <stl2/detail/concepts/object/move_constructible.hpp>

/////////////////////////////////////////////
// Comparison Concepts [concepts.lib.compare]
//
STL2_OPEN_NAMESPACE {
	///////////////////////////////////////////////////////////////////////////
	// Boolean [concepts.lib.compare.boolean]
	//
	template <class B>
	concept bool Boolean() {
		return MoveConstructible<B>() && requires (const B& b1, const B& b2, const bool a) {
			// Requirements common to both Boolean and BooleanTestable.
			STL2_BINARY_DEDUCTION_CONSTRAINT(b1, ConvertibleTo, bool);
			STL2_BINARY_DEDUCTION_CONSTRAINT(!b1, ConvertibleTo, bool);
			STL2_EXACT_TYPE_CONSTRAINT(b1 && a, bool);
			STL2_EXACT_TYPE_CONSTRAINT(b1 || a, bool);

			// Requirements of Boolean that are also be valid for
			// BooleanTestable, but for which BooleanTestable does not
			// require validation.
			STL2_EXACT_TYPE_CONSTRAINT(b1 && b2, bool);
			STL2_EXACT_TYPE_CONSTRAINT(a && b2, bool);
			STL2_EXACT_TYPE_CONSTRAINT(b1 || b2, bool);
			STL2_EXACT_TYPE_CONSTRAINT(a || b2, bool);

			// Requirements of Boolean that are not required by
			// BooleanTestable.
			STL2_BINARY_DEDUCTION_CONSTRAINT(b1 == b2, ConvertibleTo, bool);
			STL2_BINARY_DEDUCTION_CONSTRAINT(b1 == a, ConvertibleTo, bool);
			STL2_BINARY_DEDUCTION_CONSTRAINT(a == b2, ConvertibleTo, bool);
			STL2_BINARY_DEDUCTION_CONSTRAINT(b1 != b2, ConvertibleTo, bool);
			STL2_BINARY_DEDUCTION_CONSTRAINT(b1 != a, ConvertibleTo, bool);
			STL2_BINARY_DEDUCTION_CONSTRAINT(a != b2, ConvertibleTo, bool);
		};
	}

	template <class T, class U>
	concept bool __equality_comparable =
		requires (const T& t, const U& u) {
			STL2_DEDUCTION_CONSTRAINT(t == u, Boolean);
			STL2_DEDUCTION_CONSTRAINT(t != u, Boolean);
			// Axiom: t == u and t != u have the same definition space
			// Axiom: bool(t != u) == !bool(t == u)
		};

	///////////////////////////////////////////////////////////////////////////
	// WeaklyEqualityComparable [concepts.lib.compare.equalitycomparable]
	// Relaxation of EqualityComparable<T, U> that doesn't require
	// EqualityComparable<T>, EqualityComparable<U>, Common<T, U>, or
	// EqualityComparable<common_type_t<T, U>>. I.e., provides exactly the
	// requirements for Sentinel's operator ==.
	//
	template <class T, class U>
	concept bool WeaklyEqualityComparable() {
		return __equality_comparable<T, U> &&
			__equality_comparable<U, T>;
		// Axiom: u == t and t == u have the same definition space
		// Axiom: bool(u == t) == bool(t == u)
	}

	///////////////////////////////////////////////////////////////////////////
	// EqualityComparable [concepts.lib.compare.equalitycomparable]
	//
	template <class T>
	concept bool EqualityComparable() {
		return WeaklyEqualityComparable<T, T>();
	}

	template <class T, class U>
	concept bool EqualityComparable() {
		return
			EqualityComparable<T>() &&
			EqualityComparable<U>() &&
			WeaklyEqualityComparable<T, U>() &&
			CommonReference<const T&, const U&>() &&
			EqualityComparable<__uncvref<common_reference_t<const T&, const U&>>>();
	}

	///////////////////////////////////////////////////////////////////////////
	// StrictTotallyOrdered [concepts.lib.compare.stricttotallyordered]
	//
	template <class T, class U>
	concept bool __totally_ordered = requires (const T& t, const U& u) {
		STL2_DEDUCTION_CONSTRAINT(t < u, Boolean);
		STL2_DEDUCTION_CONSTRAINT(t > u, Boolean);
		STL2_DEDUCTION_CONSTRAINT(t <= u, Boolean);
		STL2_DEDUCTION_CONSTRAINT(t >= u, Boolean);
		// Axiom: t < u, t > u, t <= u, t >= u have the same definition space.
		// Axiom: If bool(t < u) then bool(t <= u)
		// Axiom: If bool(t > u) then bool(t >= u)
		// Axiom: Exactly one of bool(t < u), bool(t > u), or
		//        (bool(t <= u) && bool(t >= u)) is true
	};

	template <class T>
	concept bool StrictTotallyOrdered() {
		return EqualityComparable<T>() && __totally_ordered<T, T>;
	}

	template <class T, class U>
	concept bool StrictTotallyOrdered() {
		return
			StrictTotallyOrdered<T>() &&
			StrictTotallyOrdered<U>() &&
			EqualityComparable<T, U>() &&
			__totally_ordered<T, U> &&
			__totally_ordered<U, T> &&
			CommonReference<const T&, const U&>() &&
			StrictTotallyOrdered<__uncvref<common_reference_t<const T&, const U&>>>();
	}
} STL2_CLOSE_NAMESPACE

#endif
