<<<<<<< HEAD
﻿// Copyright (c) Microsoft Corporation.
=======
// Copyright (c) Microsoft Corporation.
>>>>>>> 11e7657 (DFS_Strategy)
// Licensed under the MIT License.

#ifndef COYOTE_STRATEGY_H
#define COYOTE_STRATEGY_H

<<<<<<< HEAD
namespace coyote
{
    enum class Strategy
    {
        None = 0,
        Random
    };
}

#endif // COYOTE_STRATEGY_H
=======
#include "../operations/operations.h"
#include <string>

namespace coyote
{
	class Strategy
	{
	public:
		// Returns the next operation.
		virtual size_t next_operation(Operations& operations) = 0;

		// Returns the next boolean choice.
		virtual bool next_boolean() = 0;

		// Returns the next integer choice.
		virtual int next_integer(int max_value) = 0;

		// Returns the seed used in the current iteration.
		virtual size_t seed() = 0;

		// Prepares the next iteration.
		virtual void prepare_next_iteration() = 0;
	};
}

<<<<<<< HEAD
#endif
>>>>>>> 11e7657 (DFS_Strategy)
=======
#endif // COYOTE_STRATEGY_H
>>>>>>> c869885 (edits)
