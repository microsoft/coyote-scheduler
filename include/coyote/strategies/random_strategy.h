// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_RANDOM_STRATEGY_H
#define COYOTE_RANDOM_STRATEGY_H

#include "random.h"
#include "strategy.h"
#include "../settings.h"

namespace coyote
{
	class RandomStrategy : public Strategy
	{
	private:
		// The pseudo-random generator.
		Random generator;

		// The seed used by the current iteration.
		uint64_t iteration_seed;

	public:
		RandomStrategy(Settings* settings) noexcept;

		RandomStrategy(RandomStrategy&& strategy) = delete;
		RandomStrategy(RandomStrategy const&) = delete;

		RandomStrategy& operator=(RandomStrategy&& strategy) = delete;
		RandomStrategy& operator=(RandomStrategy const&) = delete;

		// Returns the next operation.
		int next_operation(Operations& operations, size_t current);

		// Returns the next boolean choice.
		bool next_boolean();

		// Returns the next integer choice.
		int next_integer(int max_value);

		// Returns the seed used in the current iteration.
		uint64_t random_seed();

		// Prepares the next iteration.
		void prepare_next_iteration(size_t iteration);
	};
}

#endif // COYOTE_RANDOM_STRATEGY_H
