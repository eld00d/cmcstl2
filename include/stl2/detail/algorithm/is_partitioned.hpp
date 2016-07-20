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
#ifndef STL2_DETAIL_ALGORITHM_IS_PARTITIONED_HPP
#define STL2_DETAIL_ALGORITHM_IS_PARTITIONED_HPP

#include <stl2/functional.hpp>
#include <stl2/iterator.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/algorithm/find_if_not.hpp>
#include <stl2/detail/algorithm/none_of.hpp>
#include <stl2/detail/concepts/callable.hpp>

///////////////////////////////////////////////////////////////////////////
// is_partitioned [alg.partitions]
//
STL2_OPEN_NAMESPACE {
	template <InputIterator I, Sentinel<I> S, class Pred, class Proj = identity>
	requires
		IndirectCallablePredicate<__f<Pred>, projected<I, __f<Proj>>>()
	bool is_partitioned(I first, S last, Pred&& pred_, Proj&& proj_ = Proj{})
	{
		auto pred = ext::make_callable_wrapper(__stl2::forward<Pred>(pred_));
		auto proj = ext::make_callable_wrapper(__stl2::forward<Proj>(proj_));
		first = __stl2::find_if_not(__stl2::move(first), last,
			__stl2::ref(pred), __stl2::ref(proj));
		return __stl2::none_of(__stl2::move(first), __stl2::move(last),
			__stl2::ref(pred), __stl2::ref(proj));
	}

	template <InputRange Rng, class Pred, class Proj = identity>
	requires
		IndirectCallablePredicate<
			__f<Pred>, projected<iterator_t<Rng>, __f<Proj>>>()
	bool is_partitioned(Rng&& rng, Pred&& pred, Proj&& proj = Proj{})
	{
		return __stl2::is_partitioned(__stl2::begin(rng), __stl2::end(rng),
			__stl2::forward<Pred>(pred), __stl2::forward<Proj>(proj));
	}

	// Extension
	template <class E, class Pred, class Proj = identity>
	requires
		IndirectCallablePredicate<__f<Pred>, projected<const E*, __f<Proj>>>()
	bool is_partitioned(std::initializer_list<E>&& rng, Pred&& pred, Proj&& proj = Proj{})
	{
		return __stl2::is_partitioned(__stl2::begin(rng), __stl2::end(rng),
			__stl2::forward<Pred>(pred), __stl2::forward<Proj>(proj));
	}
} STL2_CLOSE_NAMESPACE

#endif
