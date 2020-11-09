// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_RANDOM_STRATEGY_H
#define COYOTE_RANDOM_STRATEGY_H

<<<<<<< HEAD
<<<<<<< HEAD:include/coyote/strategies/random_strategy.h
#include "random.h"
<<<<<<< HEAD
#include "../settings.h"
#include "../operations/operations.h"
=======
#include "strategy.h"
>>>>>>> 11e7657 (DFS_Strategy)
=======
#include "../random.h"
#include "../strategy.h"
<<<<<<< HEAD
>>>>>>> 212cb6d (DFS_Strategy):include/coyote/strategies/Probabilistic/random_strategy.h
=======
#include <string>
>>>>>>> b1d4a7e (DFS_Strategy)
=======
#include "random.h"
#include "strategy.h"
<<<<<<< HEAD
#include "../operations/operations.h"
>>>>>>> f6a1921 (edits)
=======
>>>>>>> c869885 (edits)

namespace coyote
{
	class RandomStrategy : public Strategy
	{
	private:
		// The pseudo-random generator.
		Random generator;

		// The seed used by the current iteration.
		uint64_t iteration_seed;

	public:
		RandomStrategy(Settings* settings) noexcept;

		RandomStrategy(RandomStrategy&& strategy) = delete;
		RandomStrategy(RandomStrategy const&) = delete;

		RandomStrategy& operator=(RandomStrategy&& strategy) = delete;
		RandomStrategy& operator=(RandomStrategy const&) = delete;

		// Returns the next operation.
		int next_operation(Operations& operations);

		// Returns the next boolean choice.
		bool next_boolean();

		// Returns the next integer choice.
		int next_integer(int max_value);

		// Returns the seed used in the current iteration.
		uint64_t random_seed();

		// Prepares the next iteration.
		void prepare_next_iteration();
	};
}

#endif // COYOTE_RANDOM_STRATEGY_H
