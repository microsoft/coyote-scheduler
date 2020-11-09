// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <iostream>
#include "error_code.h"
#include "strategies/pct_strategy.h"

namespace coyote
{
	PCTStrategy::PCTStrategy(Settings* settings) noexcept :
		iteration_seed(settings->random_seed()),
		generator(settings->random_seed()),
		max_priority_switches(settings->exploration_strategy_bound()),
		scheduled_steps(0),
		schedule_length(0)
	{
	}

	int PCTStrategy::next_operation(Operations& operations, size_t current)
	{
		set_new_operation_priorities(operations, current);
		try_deprioritize_operation_with_highest_priority(operations);
		scheduled_steps++;

		return get_operation_with_highest_priority(operations);
	}

	void PCTStrategy::set_new_operation_priorities(Operations& operations, size_t current)
	{
		if (prioritized_operations.size() == 0)
		{
			prioritized_operations.push_back(current);
			known_operations.insert(current);
#ifdef COYOTE_DEBUG_LOG
			std::cout << "[coyote::pct] assigning priority 0 for operation " << current << std::endl;
#endif // COYOTE_DEBUG_LOG
		}

		// Randomize the priority of all new operations.
		for (size_t idx = 0; idx < operations.size(); idx++)
		{
			size_t next_op = operations[idx];

			// Check if the priority of the operation is already assigned.
			auto it = known_operations.find(next_op);
			if (it == known_operations.end())
			{
				// Randomly choose a priority for this operation.
				auto index = generator.next() % (prioritized_operations.size() + 1);
				auto it = prioritized_operations.begin();
				std::advance(it, index);
				prioritized_operations.insert(it, next_op);
				known_operations.insert(next_op);
#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::pct] assigning priority " << index << " for operation " << next_op << std::endl;
#endif // COYOTE_DEBUG_LOG
			}
		}
	}

	bool PCTStrategy::try_deprioritize_operation_with_highest_priority(Operations& operations)
	{
		if (operations.size() <= 1)
		{
			// Nothing to do, there is only one enabled operation available.
			return false;
		}

		auto it = priority_change_points.find(scheduled_steps);
		if (it == priority_change_points.end())
		{
			return false;
		}

		// Deprioritize the operation by putting it in the end of the list.
		size_t op = get_operation_with_highest_priority(operations);
		prioritized_operations.remove(op);
		prioritized_operations.push_back(op);
#ifdef COYOTE_DEBUG_LOG
		std::cout << "[coyote::pct] operation " << op << " is deprioritized" << std::endl;
#endif // COYOTE_DEBUG_LOG
		return true;
	}

	size_t PCTStrategy::get_operation_with_highest_priority(Operations& operations)
	{
		for (auto it = prioritized_operations.begin(); it != prioritized_operations.end(); it++)
		{
			for (size_t idx = 0; idx < operations.size(); idx++)
			{
				size_t next_op = operations[idx];
				if (*it == next_op)
				{
					return next_op;
				}
			}
		}

		throw ErrorCode::InternalError;
	}

	bool PCTStrategy::next_boolean()
	{
		scheduled_steps++;
		return generator.next() & 1;
	}

	int PCTStrategy::next_integer(int max_value)
	{
		scheduled_steps++;
		return generator.next() % max_value;
	}

	uint64_t PCTStrategy::random_seed()
	{
		return iteration_seed;
	}

	void PCTStrategy::prepare_next_iteration(size_t iteration)
	{
		// The first iteration has no knowledge of the execution, so only initialize from the second
		// iteration and onwards. Note that although we could initialize the first length based on a
		// heuristic, its not worth it, as the strategy will typically explore thousands of iterations,
		// plus its also interesting to explore a schedule with no forced priority change points.
		if (iteration > 0)
		{
			if (schedule_length < scheduled_steps)
			{
				schedule_length = scheduled_steps;
			}

			scheduled_steps = 0;

			prioritized_operations.clear();
			known_operations.clear();
			priority_change_points.clear();

			shuffle_priority_change_points();
		}
	}

	void PCTStrategy::shuffle_priority_change_points()
	{
		std::list<int> range;
		for (int i = 1; i < schedule_length; i++)
		{
			range.push_back(i);
		}

		for (int idx = range.size() - 1; idx >= 1; idx--)
		{
			int point = generator.next() % range.size();

			std::list<int>::iterator idx_it = range.begin();
			std::advance(idx_it, idx);

			std::list<int>::iterator point_it = range.begin();
			std::advance(point_it, point);

			std::iter_swap(idx_it, point_it);
		}

		int count = 0;
		for (auto it = range.begin(); it != range.end(); it++)
		{
#ifdef COYOTE_DEBUG_LOG
			std::cout << "[coyote::pct] assigning priority change at " << *it << " step" << std::endl;
#endif // COYOTE_DEBUG_LOG
			priority_change_points.insert(*it);
			count++;
			if (count == max_priority_switches)
			{
				break;
			}
		}
	}
}
