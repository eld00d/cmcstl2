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
#ifndef STL2_DETAIL_CONCEPTS_CORE_HPP
#define STL2_DETAIL_CONCEPTS_CORE_HPP

#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>

////////////////////////////////////////
// Core Concepts [concepts.lib.corelang]
//
STL2_OPEN_NAMESPACE {
	template <template <class...> class T, class... U>
	concept bool _Valid = requires { typename T<U...>; };

	template <class U, template <class...> class T, class... V>
	concept bool _Is = _Valid<T, U, V...> && T<U, V...>::value;

	template <class U, template <class...> class T, class... V>
	concept bool _IsNot = !_Is<U, T, V...>;

	// U is a cv/ref-qualified specialization of class template T.
	template <class U, template <class...> class T>
	concept bool _SpecializationOf = meta::is<__uncvref<U>, T>::value;

	///////////////////////////////////////////////////////////////////////////
	// Same [concepts.lib.corelang.same]
	// Extension: variadic.
	//
	template <class...>
	constexpr bool __same = true;
	template <class T, class...Rest>
	constexpr bool __same<T, Rest...> =
#if defined(__GNUC__)
		(true && ... && __is_same_as(T, Rest));
#else
		(true && ... && is_same<T, Rest>::value);
#endif

	template <class...Ts>
	concept bool Same() {
		return __same<Ts...>;
	}

	template <class T>
	concept bool _Decayed = Same<T, decay_t<T>>();

	template <class T>
	concept bool _Unqual = Same<T, __uncvref<T>>();

	///////////////////////////////////////////////////////////////////////////
	// DerivedFrom [concepts.lib.corelang.derived]
	//
	template <class T, class U>
	concept bool DerivedFrom() {
#if defined(__GNUC__)
		return __is_base_of(U, T);
#else
		return _Is<U, is_base_of, T>;
#endif
	}

	///////////////////////////////////////////////////////////////////////////
	// ExplicitlyConvertibleTo [Extension]
	//
	namespace ext {
		template <class T, class U>
		concept bool ExplicitlyConvertibleTo() {
			return requires (T (&t)()) { static_cast<U>(t()); };
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// ImplicitlyConvertibleTo [Extension]
	// Equivalent to ConvertibleTo from the TS.
	//
	namespace ext {
		template <class T, class U>
		concept bool ImplicitlyConvertibleTo() {
			// Q: Why not { t } -> U ?
			// A: They do not have equivalent results as of 20150724, which is
			//    likely a bug.
			return _Is<T, is_convertible, U>;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// ConvertibleTo [concepts.lib.corelang.convertibleto]
	// Not to spec: Requires both implicit and explicit conversion with
	//              equal results.
	//
	template <class T, class U>
	concept bool ConvertibleTo() {
		return ext::ExplicitlyConvertibleTo<T, U>() &&
			ext::ImplicitlyConvertibleTo<T, U>();
		// Axiom: implicit and explicit conversion have equal results.
	}

	///////////////////////////////////////////////////////////////////////////
	// PubliclyDerivedFrom [Extension]
	//
	namespace ext {
		template <class T, class U>
		concept bool PubliclyDerivedFrom() {
			return ConvertibleTo<T, U>() && (Same<T, U>() || DerivedFrom<T, U>());
		}
	}
} STL2_CLOSE_NAMESPACE

#endif
