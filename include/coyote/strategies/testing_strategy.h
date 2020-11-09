// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_TESTING_STRATEGY_H
#define COYOTE_TESTING_STRATEGY_H

#include "strategy.h"
#include "Exhaustive/dfs_strategy.h"
#include "Probabilistic/random_strategy.h"
#include "Probabilistic/pct_strategy.h"

namespace coyote
{
	class TestingStrategy
	{
	private:
		Strategy* strategy;

	public:
		// Random Strategy
		TestingStrategy(size_t seed)
		{
			strategy = new RandomStrategy(seed);
		}

		TestingStrategy(std::string strat)
		{
			if (strat.compare("DFSStrategy") == 0)
			{
				strategy = new DFSStrategy();
			}
			else if (strat.compare("PCTStrategy") == 0)
			{
				strategy = new PCTStrategy();
			}
			else if (strat.compare("RandomStrategy") == 0)
			{
				strategy = new RandomStrategy(std::chrono::high_resolution_clock::now().time_since_epoch().count());
			}
			else
			{
				throw "Wrong or unavailable selection of testing strategy.";
			}
		}

		// Returns the next operation.
		size_t next_operation(Operations& operations)
		{
			return strategy->next_operation(operations);
		}

		// Returns the next boolean choice.
		bool next_boolean()
		{
			return strategy->next_boolean();
		}

		// Returns the next integer choice.
		int next_integer(int max_value)
		{
			return strategy->next_integer(max_value);
		}

		// Prepares the next iteration.
		void prepare_next_iteration()
		{
			return strategy->prepare_next_iteration();
		}
	};
}

#endif
