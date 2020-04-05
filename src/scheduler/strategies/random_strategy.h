// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef RANDOM_STRATEGY_H
#define RANDOM_STRATEGY_H

#include <optional>
#include <random>
#include "operations/operation.h"

namespace coyote
{
	class RandomStrategy
	{
	private:
		// A Mersenne Twister pseudo-random generator.
		std::mt19937 generator;

		// The seed used by the current iteration.
		size_t iteration_seed;

	public:
		RandomStrategy(size_t seed) noexcept;
		RandomStrategy(RandomStrategy&& strategy) = delete;
		RandomStrategy(RandomStrategy const&) = delete;

		RandomStrategy& operator=(RandomStrategy&& strategy) = delete;
		RandomStrategy& operator=(RandomStrategy const&) = delete;

		// Returns the next operation.
		std::optional<size_t> next_operation(const std::map<size_t, std::shared_ptr<Operation>>& operations);

		// Returns the next boolean choice.
		bool next_boolean();

		// Returns the next integer choice.
		int next_integer(int max_value);

		// Returns the seed used in the current iteration.
		size_t seed();

		// Prepares the next iteration.
		void prepare_next_iteration();
	};
}

#endif // RANDOM_STRATEGY_H
