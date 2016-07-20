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
#ifndef STL2_DETAIL_CONCEPTS_OBJECT_MOVE_CONSTRUCTIBLE_HPP
#define STL2_DETAIL_CONCEPTS_OBJECT_MOVE_CONSTRUCTIBLE_HPP

#include <stl2/type_traits.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>
#include <stl2/detail/concepts/core.hpp>

STL2_OPEN_NAMESPACE {
	///////////////////////////////////////////////////////////////////////////
	// Addressable [Extension]
	//
	namespace ext {
		template <class T>
		concept bool Addressable() {
			return requires (T& t, const T& ct) {
				STL2_EXACT_TYPE_CONSTRAINT(&t, T*);
				STL2_EXACT_TYPE_CONSTRAINT(&ct, const T*);
				// Axiom: &ct == addressof(ct)
			};
		}
	}

	namespace detail {
		template <class>
		constexpr bool is_complete_object = false;

		template <_Is<is_object> T>
		requires
			sizeof(T) > 0
		constexpr bool is_complete_object<T> = true;

		template <class T>
		constexpr bool MustBeComplete() {
			static_assert(is_complete_object<T>, "Concept check for incomplete type T.");
			return true;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Destructible [concepts.lib.object.destructible]
	// Not to spec: is_object and accepting the parameters by reference are
	//              necessary to prevent hard errors in the requires clause
	//              with odd types.
	//
	template <class T>
	concept bool Destructible() {
		return ext::Addressable<T>() &&
			_Is<T, is_object> &&
			_IsNot<T, is_array> &&
			detail::MustBeComplete<T>() &&
			requires (T& t, T* const p) {
				{ t.~T() } noexcept;
				delete p;
				delete[] p;
			};
	}

	///////////////////////////////////////////////////////////////////////////
	// Constructible [concepts.lib.object.constructible]
	// Extension: ConstructibleObject and BindableReference are user-visible.
	//
	namespace ext {
		template <class T, class...Args>
		concept bool ConstructibleObject2 = requires (Args&&...args) {
			T{ (Args&&)args... };
			new T{ (Args&&)args... };
		};

		template <class T, class...Args>
		concept bool ConstructibleObject =
			Destructible<T>() && ConstructibleObject2<T, Args...>;

		// 20150718: Not to spec: spec is broken.
		// FIXME: Correct wording.
		template <class T, class...Args>
		concept bool BindableReference =
			_Is<T, is_reference> &&
			// requires (Args&&...args) { T{ (Args&&)args... }; };
			// requires (Args&&...args) { T( (Args&&)args... ); };
			_Is<T, is_constructible, Args...>;
	}

	template <class T, class...Args>
	constexpr bool __constructible =
		ext::ConstructibleObject<T, Args...> ||
		ext::BindableReference<T, Args...>;

	template <class T, class...Args>
	concept bool Constructible() {
		return __constructible<T, Args...>;
	}

	///////////////////////////////////////////////////////////////////////////
	// DefaultConstructible [concepts.lib.object.defaultconstructible]
	// Note: Due to implementation variance around DR1518, may not correctly
	// enforce the requirement that the default constructor be non-explicit.
	//
	template <class T>
	concept bool DefaultConstructible() {
		return Constructible<T>() && requires (const std::size_t n) {
			new T[n]{}; // not required to be equality preserving
		};
	}

	///////////////////////////////////////////////////////////////////////////
	// MoveConstructible [concepts.lib.object.moveconstructible]
	//
	template <class T>
	concept bool MoveConstructible() {
		return Constructible<T, remove_cv_t<T>>() &&
			ext::ImplicitlyConvertibleTo<remove_cv_t<T>, T>();
	}
} STL2_CLOSE_NAMESPACE

#endif
