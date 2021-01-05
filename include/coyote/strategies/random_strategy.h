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

		// The probability of deviating from the current operation if it is enabled.
		size_t scheduling_deviation_probability;

	public:
		RandomStrategy(Settings* settings) noexcept :
			generator(settings->random_seed()),
			iteration_seed(settings->random_seed()),
			scheduling_deviation_probability(settings->exploration_strategy_bound())
		{
		}

		RandomStrategy(RandomStrategy&& strategy) = delete;
		RandomStrategy(RandomStrategy const&) = delete;

		RandomStrategy& operator=(RandomStrategy&& strategy) = delete;
		RandomStrategy& operator=(RandomStrategy const&) = delete;

		// Returns the next operation.
		int next_operation(Operations& operations, size_t current)
		{
			if (scheduling_deviation_probability < 100)
			{
				bool isCurrentEnabled = false;
				for (size_t idx = 0; idx < operations.size(); idx++)
				{
					if (operations[idx] == current)
					{
						isCurrentEnabled = true;
						break;
					}
				}

				if (isCurrentEnabled && (generator.next() % 100) > scheduling_deviation_probability)
				{
					return (int)current;
				}
			}

			return (int)operations[generator.next() % operations.size()];
		}

		// Returns the next boolean choice.
		bool next_boolean()
		{
			return (generator.next() & 1) == 0;
		}

		// Returns the next integer choice.
		int next_integer(int max_value)
		{
			return generator.next() % max_value;
		}

		// Returns the seed used in the current iteration.
		uint64_t random_seed()
		{
			return iteration_seed;
		}

		// Prepares the next iteration.
		void prepare_next_iteration(size_t iteration)
		{
			iteration_seed += 1;
			generator.seed(iteration_seed);
		}
	};
}

#endif // COYOTE_RANDOM_STRATEGY_H
