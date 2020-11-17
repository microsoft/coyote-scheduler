// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "strategies/probabilistic_strategy.h"

namespace coyote
{
	ProbabilisticStrategy::ProbabilisticStrategy(Settings* settings) noexcept :
		iteration_seed(settings->random_seed()),
		generator(settings->random_seed()),
		difficulty(settings->exploration_strategy_bound())
	{
	}

	size_t ProbabilisticStrategy::next_operation(Operations& operations, size_t current)
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

		if (isCurrentEnabled)
		{
			bool change = true;
			for (int idx = 0; idx < difficulty; idx++)
			{
				if ((generator.next() & 1) == 0)
				{
					change = false;
					break;
				}
			}

			if (!change)
			{
				return current;
			}
		}

		return operations[generator.next() % operations.size()];
	}

	bool ProbabilisticStrategy::next_boolean()
	{
		return (generator.next() & 1) == 0;
	}

	int ProbabilisticStrategy::next_integer(int max_value)
	{
		return generator.next() % max_value;
	}

	uint64_t ProbabilisticStrategy::random_seed()
	{
		return iteration_seed;
	}

	void ProbabilisticStrategy::prepare_next_iteration(size_t iteration)
	{
		iteration_seed += 1;
		generator.seed(iteration_seed);
	}
}
