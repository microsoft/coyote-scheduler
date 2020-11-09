// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_PCT_STRATEGY_H
#define COYOTE_PCT_STRATEGY_H

#include "random.h"
#include "strategy.h"
#include <list>
#include <set>
#include <chrono>

namespace coyote
{
	class PCTStrategy : public Strategy
	{
	private:
		// The pseudo-random generator.
		Random generator;

		// The seed used by the current iteration.
		size_t iteration_seed;

		// Max number of priority switch points.
		int max_priority_switch_points;

		// Current scheduling index (next sch point)
		int scheduled_steps;

		// Approximate length of the schedule across all iterations.
		int schedule_length;

		// List of prioritized operations.
		std::list<size_t>* prioritized_operations;

		// Set of priority change points.
		std::set<int>* priority_change_points;

	public:
		PCTStrategy(size_t seed, int maxPrioritySwitchPoints = 2) noexcept;

		PCTStrategy(PCTStrategy&& strategy) = delete;
		PCTStrategy(PCTStrategy const&) = delete;

		PCTStrategy& operator=(PCTStrategy&& strategy) = delete;
		PCTStrategy& operator=(PCTStrategy const&) = delete;

		// Returns the next operation.
		size_t next_operation(Operations& operations);

		// Returns the next boolean choice.
		bool next_boolean();

		// Returns the next integer choice.
		int next_integer(int max_value);

		// Prepares the next iteration.
		void prepare_next_iteration();

	private:
		// Retrun the prioritized operation
		size_t get_prioritized_operation(std::vector<size_t> enabled_oprs);

		// Return the highest priority enabled operation
		size_t get_highest_priority_enabled_operation(std::vector<size_t> enabled_oprs);

		// Updates the priority change point to some other point (forward)
		void move_priority_change_point_forward();
	};
}

#endif // COYOTE_DFS_STRATEGY_H
