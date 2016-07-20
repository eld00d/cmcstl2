// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Eric Niebler 2015
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
#include <algorithm>
#include <numeric>
#include <stl2/iterator.hpp>
#include "../simple_test.hpp"
#include "../test_iterators.hpp"

namespace ranges = ::__stl2;

int main() {
	{
		static_assert(
				ranges::ForwardIterator<
					ranges::common_iterator<
						bidirectional_iterator<const char *>,
						sentinel<const char *>>>(),
				"");
		static_assert(
				!ranges::BidirectionalIterator<
					ranges::common_iterator<
						bidirectional_iterator<const char *>,
						sentinel<const char *>>>(),
				"");
		static_assert(
			std::is_same<
				ranges::common_reference<
					ranges::common_iterator<
						bidirectional_iterator<const char *>,
						sentinel<const char *>
					>&,
					ranges::common_iterator<
						bidirectional_iterator<const char *>,
						sentinel<const char *>
					>
				>::type,
				ranges::common_iterator<
					bidirectional_iterator<const char *>,
					sentinel<const char *>
				>
			>::value, ""
		);
		// Sized iterator range tests
		static_assert(
			!ranges::SizedSentinel<
				ranges::common_iterator<
					forward_iterator<int*>,
					sentinel<int*, true> >,
				ranges::common_iterator<
					forward_iterator<int*>,
					sentinel<int*, true> > >(),
				"");
		static_assert(
			ranges::SizedSentinel<
				ranges::common_iterator<
					random_access_iterator<int*>,
					sentinel<int*, true> >,
				ranges::common_iterator<
					random_access_iterator<int*>,
					sentinel<int*, true> > >(),
				"");
		static_assert(
			!ranges::SizedSentinel<
				ranges::common_iterator<
					random_access_iterator<int*>,
					sentinel<int*, false> >,
				ranges::common_iterator<
					random_access_iterator<int*>,
					sentinel<int*, false> > >(),
				"");
	}
	{
		int rgi[] {0,1,2,3,4,5,6,7,8,9};
		using CI = ranges::common_iterator<
			random_access_iterator<int*>,
			sentinel<int*>>;
		CI first{random_access_iterator<int*>{rgi}};
		CI last{sentinel<int*>{rgi+10}};
		CHECK(std::accumulate(first, last, 0, std::plus<int>{}) == 45);
	}
	return test_result();
}
