// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "strategies/random_strategy.h"

namespace coyote
{
	RandomStrategy::RandomStrategy(Settings* settings) noexcept :
		iteration_seed(settings->random_seed()),
		generator(settings->random_seed()),
		scheduling_deviation_probability(settings->exploration_strategy_bound())
	{
	}

	size_t RandomStrategy::next_operation(Operations& operations, size_t current)
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
				return current;
			}
		}

		return operations[generator.next() % operations.size()];
	}

	bool RandomStrategy::next_boolean()
	{
		return (generator.next() & 1) == 0;
	}

	int RandomStrategy::next_integer(int max_value)
	{
		return generator.next() % max_value;
	}

	uint64_t RandomStrategy::random_seed()
	{
		return iteration_seed;
	}

	void RandomStrategy::prepare_next_iteration(size_t iteration)
	{
		iteration_seed += 1;
		generator.seed(iteration_seed);
	}
}
