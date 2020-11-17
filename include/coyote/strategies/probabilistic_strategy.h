// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_PROBABILISTIC_STRATEGY_H
#define COYOTE_PROBABILISTIC_STRATEGY_H

#include "random.h"
#include "strategy.h"
#include "../settings.h"

namespace coyote
{
	class ProbabilisticStrategy : public Strategy
	{
	private:
		// The pseudo-random generator.
		Random generator;

		// The seed used by the current iteration.
		uint64_t iteration_seed;

		// The difficulty to context switch.
		size_t difficulty;

	public:
		ProbabilisticStrategy(Settings* settings) noexcept;

		ProbabilisticStrategy(ProbabilisticStrategy&& strategy) = delete;
		ProbabilisticStrategy(ProbabilisticStrategy const&) = delete;

		ProbabilisticStrategy& operator=(ProbabilisticStrategy&& strategy) = delete;
		ProbabilisticStrategy& operator=(ProbabilisticStrategy const&) = delete;

		// Returns the next operation.
		size_t next_operation(Operations& operations, size_t current);

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

#endif // COYOTE_PROBABILISTIC_STRATEGY_H
