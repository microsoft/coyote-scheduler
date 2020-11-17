// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_PCT_STRATEGY_H
#define COYOTE_PCT_STRATEGY_H

#include <list>
#include <set>
#include <unordered_set>
#include "random.h"
#include "strategy.h"
#include "../settings.h"

namespace coyote
{
	class PCTStrategy : public Strategy
	{
	private:
		// The pseudo-random generator.
		Random generator;

		// The seed used by the current iteration.
		uint64_t iteration_seed;

		// Max number of priority switches during one iteration.
		size_t max_priority_switches;

		// List of prioritized operations.
		std::list<size_t> prioritized_operations;

		// Set of operations with a known priority.
		std::unordered_set<size_t> known_operations;

		// Set of priority change points.
		std::set<size_t> priority_change_points;

		//Number of scheduling steps during the current iteration.
		size_t scheduled_steps;

		// Approximate length of the schedule across all iterations.
		size_t schedule_length;

	public:
		PCTStrategy(Settings* settings) noexcept;

		PCTStrategy(PCTStrategy&& strategy) = delete;
		PCTStrategy(PCTStrategy const&) = delete;

		PCTStrategy& operator=(PCTStrategy&& strategy) = delete;
		PCTStrategy& operator=(PCTStrategy const&) = delete;

		// Returns the next operation.
		int next_operation(Operations& operations, size_t current);

		// Returns the next boolean choice.
		bool next_boolean();

		// Returns the next integer choice.
		int next_integer(int max_value);

		// Returns the seed used in the current iteration.
		uint64_t random_seed();

		// Prepares the next iteration.
		void prepare_next_iteration(size_t iteration);

	private:
		// Sets the priority of new operations, if there are any.
		void set_new_operation_priorities(Operations& operations, size_t current);
		
		// Deprioritizes the operation with the highest priority, if there is a priority change point.
		bool try_deprioritize_operation_with_highest_priority(Operations& operations);

		// Returns the operation with the highest priority.
		size_t get_operation_with_highest_priority(Operations& operations);

		// Shuffles the priority change points using the Fisher-Yates algorithm.
		void shuffle_priority_change_points();
	};
}

#endif // COYOTE_PCT_STRATEGY_H
