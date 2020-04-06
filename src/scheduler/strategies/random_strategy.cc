// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <cassert>
#include <iostream>
#include <map>
#include "random_strategy.h"

namespace coyote
{
	RandomStrategy::RandomStrategy(size_t seed) noexcept :
		iteration_seed(seed),
		generator(seed)
	{
	}

	size_t RandomStrategy::next_operation(const std::vector<std::shared_ptr<Operation>>& operations)
	{
		assert(!operations.empty());
		const std::uniform_int_distribution<int> distribution(0, operations.size() - 1);
		const size_t index = distribution(generator);
		return operations[index]->id;
		//return operations[0]->id;
	}

	bool RandomStrategy::next_boolean()
	{
		const std::uniform_int_distribution<int> distribution(0, 1);
		return distribution(generator);
	}

	int RandomStrategy::next_integer(int max_value)
	{
		const std::uniform_int_distribution<int> distribution(0, max_value - 1);
		return distribution(generator);
	}

	size_t RandomStrategy::seed()
	{
		return iteration_seed;
	}

	void RandomStrategy::prepare_next_iteration()
	{
		iteration_seed += 1;
		generator.seed(iteration_seed);
	}
}
