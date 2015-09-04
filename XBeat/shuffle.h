#pragma once


namespace Urho3D {
	template <class RandomIt, class UniformRandomNumberGenerator>
	void random_shuffle(RandomIt first, RandomIt last, UniformRandomNumberGenerator&& g)
	{
		typedef typename std::uniform_int_distribution<int> distr_t;
		typedef typename distr_t::param_type param_t;

		distr_t D;
		int i, n;
		n = last - first;
		for (i = n - 1; i > 0; --i) {
			Urho3D::Swap(first + i, first + D(g, param_t(0, i)));
		}
	}
}
