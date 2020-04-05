// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

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

	std::optional<size_t> RandomStrategy::next_operation(const std::map<size_t, std::shared_ptr<Operation>>& operations)
	{
		std::vector<size_t> enabled_ops;
		for (auto& op : operations)
		{
			if (op.second->status == OperationStatus::Enabled)
			{
				enabled_ops.push_back(op.first);
			}
		}

		if (enabled_ops.empty())
		{
			return std::nullopt;
		}

		//const std::uniform_int_distribution<int> distribution(0, enabled_ops.size() - 1);
		//const size_t index = distribution(generator);
		//return enabled_ops[index];
		return enabled_ops[0];
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
