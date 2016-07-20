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
#ifndef STL2_DETAIL_IOSTREAM_CONCEPTS_HPP
#define STL2_DETAIL_IOSTREAM_CONCEPTS_HPP

#include <iosfwd>
#include <stl2/detail/fwd.hpp>

STL2_OPEN_NAMESPACE {
	///////////////////////////////////////////////////////////////////////////
	// StreamExtractable [Extension]
	//
	namespace ext {
		template <class T, class Stream = std::istream>
		concept bool StreamExtractable = requires (Stream& is, T& t) {
			STL2_EXACT_TYPE_CONSTRAINT(is >> t, Stream&);
			// Axiom: &is == &(is << t)
		};
	}

	///////////////////////////////////////////////////////////////////////////
	// StreamInsertable [Extension]
	//
	namespace ext {
		template <class T, class Stream = std::ostream>
		concept bool StreamInsertable = requires (Stream& os, const T& t) {
			STL2_EXACT_TYPE_CONSTRAINT(os << t, Stream&);
			// Axiom: &os == &(os << t)
		};
	}
} STL2_CLOSE_NAMESPACE

#endif
