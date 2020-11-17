// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_STRATEGY_H
#define COYOTE_STRATEGY_H

#include "../operations/operations.h"

namespace coyote
{
	class Strategy
	{
	public:
		// Returns the next operation.
		virtual int next_operation(Operations& operations, size_t current) = 0;

		// Returns the next boolean choice.
		virtual bool next_boolean() = 0;

		// Returns the next integer choice.
		virtual int next_integer(int max_value) = 0;

		// Returns the seed used in the current iteration.
		virtual uint64_t random_seed() = 0;

		// Prepares the next iteration.
		virtual void prepare_next_iteration(size_t iteration) = 0;

		virtual ~Strategy() = default;
	};
}

#endif // COYOTE_STRATEGY_H
