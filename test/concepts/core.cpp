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
#include "validate.hpp"

#if VALIDATE_RANGES
namespace ns {
	template <class T, class U>
	using common_type_t = ranges::common_type_t<T, U>;
}

#elif VALIDATE_STL2
#include <stl2/detail/concepts/core.hpp>
#include <stl2/utility.hpp>

namespace ns {
	using __stl2::common_type_t;
}
#endif

#include <iostream>
#include <type_traits>

#include "../simple_test.hpp"

CONCEPT_ASSERT(ranges::Same<>());
CONCEPT_ASSERT(ranges::Same<int, int>());
CONCEPT_ASSERT(ranges::Same<double, double>());
CONCEPT_ASSERT(ranges::Same<double>());
CONCEPT_ASSERT(!ranges::Same<double, int>());
CONCEPT_ASSERT(!ranges::Same<int, double>());

CONCEPT_ASSERT(ranges::Same<int, int, int, int>());
CONCEPT_ASSERT(!ranges::Same<int, int, double, int>());

#if VALIDATE_STL2
namespace publicly_derived_from_test {

struct A {};
struct B : A {};
struct C : A {};
struct D : B, C {};

CONCEPT_ASSERT(ranges::ext::PubliclyDerivedFrom<int,int>());
CONCEPT_ASSERT(ranges::ext::PubliclyDerivedFrom<A,A>());
CONCEPT_ASSERT(ranges::ext::PubliclyDerivedFrom<B,B>());
CONCEPT_ASSERT(ranges::ext::PubliclyDerivedFrom<C,C>());
CONCEPT_ASSERT(ranges::ext::PubliclyDerivedFrom<D,D>());
CONCEPT_ASSERT(ranges::ext::PubliclyDerivedFrom<B,A>());
CONCEPT_ASSERT(ranges::ext::PubliclyDerivedFrom<C,A>());
CONCEPT_ASSERT(!ranges::ext::PubliclyDerivedFrom<A,B>());
CONCEPT_ASSERT(!ranges::ext::PubliclyDerivedFrom<A,C>());
CONCEPT_ASSERT(!ranges::ext::PubliclyDerivedFrom<A,D>());
CONCEPT_ASSERT(!ranges::ext::PubliclyDerivedFrom<D,A>());
CONCEPT_ASSERT(!ranges::ext::PubliclyDerivedFrom<int,void>());
}
#endif

namespace convertible_to_test {
struct A {};
struct B : A {};

CONCEPT_ASSERT(ranges::ConvertibleTo<A, A>());
CONCEPT_ASSERT(ranges::ConvertibleTo<B, B>());
CONCEPT_ASSERT(!ranges::ConvertibleTo<A, B>());
CONCEPT_ASSERT(ranges::ConvertibleTo<B, A>());
CONCEPT_ASSERT(ranges::ConvertibleTo<int, double>());
CONCEPT_ASSERT(ranges::ConvertibleTo<double, int>());
}

namespace common_test {
CONCEPT_ASSERT(ranges::Same<ns::common_type_t<int, int>, int>());
//CONCEPT_ASSERT(ranges::Same<ns::common_type_t<int, float, double>, double>());

CONCEPT_ASSERT(ranges::Common<int, int>());
CONCEPT_ASSERT(ranges::Common<int, double>());
CONCEPT_ASSERT(ranges::Common<double, int>());
CONCEPT_ASSERT(ranges::Common<double, double>());
CONCEPT_ASSERT(!ranges::Common<void, int>());
CONCEPT_ASSERT(!ranges::Common<int*, int>());
CONCEPT_ASSERT(ranges::Common<void*, int*>());
CONCEPT_ASSERT(ranges::Common<double,long long>());
//CONCEPT_ASSERT(ranges::Common<int, float, double>());
//CONCEPT_ASSERT(!ranges::Common<int, float, double, void>());

struct B {};
struct C { C() = default; C(B) {} C(int) {} };
CONCEPT_ASSERT(ranges::Common<B,C>());
//CONCEPT_ASSERT(ranges::Common<int, C, B>());

struct incomplete;
CONCEPT_ASSERT(ranges::Common<void*, incomplete*>());
}

namespace {
struct tag {};
struct A { A() = default; A(int) {} explicit A(tag) {} };
struct B : A {};
struct C : B {};

enum class result {
	exact, publicly_derived, convertible,
	explicitly_convertible, unrelated
};

#if VALIDATE_STL2
// Concepts version
result f(A) {
	std::cout << "exactly A\n";
	return result::exact;
}

result f(__stl2::ext::PubliclyDerivedFrom<A>) {
	std::cout << "Publicly derived from A\n";
	return result::publicly_derived;
}

result f(__stl2::ConvertibleTo<A>) {
	std::cout << "Implicitly convertible to A\n";
	return result::convertible;
}

result f(__stl2::ext::ExplicitlyConvertibleTo<A>) {
	std::cout << "Explicitly convertible to A\n";
	return result::explicitly_convertible;
}

result f(auto) {
	std::cout << "Nothing to do with A\n";
	return result::unrelated;
}

#else
// C++11 version
result f(A) {
	std::cout << "exactly A\n";
	return result::exact;
}

template <class T>
meta::if_c<
	std::is_base_of<A, T>::value &&
		std::is_convertible<T,A>::value &&
		!std::is_same<A,T>::value,
	 result>
f(T) {
	std::cout << "Publicly derived from A\n";
	return result::publicly_derived;
}

template <class T>
meta::if_c<
	std::is_convertible<T,A>::value &&
		!(std::is_base_of<A,T>::value ||
			std::is_same<A,T>::value),
	result>
f(T) {
	std::cout << "Implicitly convertible to A\n";
	return result::convertible;
}

template <class T>
meta::if_c<
	std::is_constructible<A,T>::value &&
		!(std::is_convertible<T,A>::value ||
			std::is_base_of<A,T>::value ||
			std::is_same<A,T>::value),
	result>
f(T) {
	std::cout << "Explicitly convertible to A\n";
	return result::explicitly_convertible;
}

template <class T>
meta::if_c<
	!(std::is_constructible<A,T>::value ||
		std::is_convertible<T,A>::value ||
		std::is_base_of<A,T>::value ||
		std::is_same<A,T>::value),
	result>
f(T) {
	std::cout << "Nothing to do with A\n";
	return result::unrelated;
}
#endif
} // unnamed namespace

int main() {
	CHECK(f(A{}) == result::exact);
	{ const A a{}; CHECK(f(a) == result::exact); }
	CHECK(f(B{}) == result::publicly_derived);
	{ const B b{}; CHECK(f(b) == result::publicly_derived); }
	CHECK(f(42) == result::convertible);
	CHECK(f(tag{}) == result::explicitly_convertible);
	CHECK(f("foo") == result::unrelated);

	return ::test_result();
}
