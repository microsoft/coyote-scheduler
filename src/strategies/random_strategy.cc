// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "strategies/random_strategy.h"

namespace coyote
{
	RandomStrategy::RandomStrategy(Settings* settings) noexcept :
		iteration_seed(settings->random_seed()),
		generator(settings->random_seed())
	{
	}

	int RandomStrategy::next_operation(Operations& operations)
	{
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

	void RandomStrategy::prepare_next_iteration()
	{
		iteration_seed += 1;
		generator.seed(iteration_seed);
	}
}
