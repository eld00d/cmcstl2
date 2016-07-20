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

#if VALIDATE_STL2
#include <stl2/detail/concepts/compare.hpp>
#endif

#include <type_traits>
#include <bitset>
#include "../simple_test.hpp"

namespace boolean_test {
// Better have at least these three, since we use them as
// examples in the TS draft.
CONCEPT_ASSERT(ranges::Boolean<bool>());
CONCEPT_ASSERT(ranges::Boolean<std::true_type>());
CONCEPT_ASSERT(ranges::Boolean<std::bitset<42>::reference>());

CONCEPT_ASSERT(ranges::Boolean<int>());
CONCEPT_ASSERT(!ranges::Boolean<void*>());

struct A {};
struct B { operator bool() const; };

CONCEPT_ASSERT(!ranges::Boolean<A>());
CONCEPT_ASSERT(ranges::Boolean<B>());
}

namespace equality_comparable_test {
struct A {
	friend bool operator==(const A&, const A&);
	friend bool operator!=(const A&, const A&);
};

CONCEPT_ASSERT(ranges::EqualityComparable<int>());
CONCEPT_ASSERT(ranges::EqualityComparable<A>());
CONCEPT_ASSERT(!ranges::EqualityComparable<void>());

CONCEPT_ASSERT(ranges::EqualityComparable<int, int>());
CONCEPT_ASSERT(ranges::EqualityComparable<A, A>());
CONCEPT_ASSERT(!ranges::EqualityComparable<void, void>());
} // namespace equality_comparable_test

CONCEPT_ASSERT(ranges::StrictTotallyOrdered<int>());
CONCEPT_ASSERT(ranges::StrictTotallyOrdered<float>());
CONCEPT_ASSERT(ranges::StrictTotallyOrdered<std::nullptr_t>());
CONCEPT_ASSERT(!ranges::StrictTotallyOrdered<void>());

CONCEPT_ASSERT(ranges::StrictTotallyOrdered<int, int>());
CONCEPT_ASSERT(ranges::StrictTotallyOrdered<int, double>());
CONCEPT_ASSERT(!ranges::StrictTotallyOrdered<int, void>());

int main() {
	return ::test_result();
}
