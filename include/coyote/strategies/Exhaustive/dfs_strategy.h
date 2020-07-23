// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_DFS_STRATEGY_H
#define COYOTE_DFS_STRATEGY_H

#include "../strategy.h"
#include <list>
#include <map>
#include <stack>

namespace coyote
{
	class DFSStrategy : public Strategy
	{
	private:

		// Stack of scheduling choices
		std::map<int, std::stack<size_t>*>* ScheduleStack;

		// Current scheduling index (next sch point)
		int SchIndex;

		// Returns the next choice (operation or bool or integer)
		size_t next_choice(std::vector<size_t> choices);

	public:
		DFSStrategy() noexcept;

		// Returns the next operation.
		size_t next_operation(Operations& operations);

		// Returns the next boolean choice.
		bool next_boolean();

		// Returns the next integer choice.
		int next_integer(int max_value);

		// Prepares the next iteration.
		void prepare_next_iteration();

		// Description about the strategy
		std::string get_description();

		// Fair strategy or not
		bool is_fair();
	};
}

#endif // COYOTE_DFS_STRATEGY_H
