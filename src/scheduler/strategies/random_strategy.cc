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
		for (auto& op : operations)
		{
			if (op.second->status == OperationStatus::Enabled)
			{
				return op.first;
			}
		}

		return std::nullopt;

		//// Get all enabled operations.
		//std::vector<std::shared_ptr<Operation>> enabled_ops;
		//for (auto& op : operation_map)
		//{
		//	if (op.second->status == OperationStatus::Enabled)
		//	{
		//		enabled_ops.push_back(std::shared_ptr<Operation>(op.second));
		//	}
		//}

		//if (operations.empty())
		//{
		//	return nullptr;
		//}

		//std::uniform_int_distribution<int> distribution(0, operations.size());
		//const size_t index = distribution(generator);
		//return operations[index];
	}

	bool RandomStrategy::next_boolean()
	{
		std::uniform_int_distribution<int> distribution(0, 1);
		return distribution(generator);
	}

	int RandomStrategy::next_integer(int max_value)
	{
		std::uniform_int_distribution<int> distribution(0, max_value - 1);
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
