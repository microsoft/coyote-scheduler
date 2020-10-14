// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <cstdint>
#include "strategies/random.h"

namespace coyote
{
	Random::Random(size_t seed) noexcept :
		state_x(seed),
		state_y(seed == 0 ? 5489 : 0)
	{
		next();
	}

	void Random::seed(const size_t seed)
	{
		state_x = seed;
		state_y = seed == 0 ? 5489 : 0;
		next();
	}

	size_t Random::next()
	{
		const size_t x = state_x;
		size_t y = state_y;
		const size_t result = state_x + y;

		y ^= x;
		state_x = rotl(x, 24) ^ y ^ (y << 16);
		state_y = rotl(y, 37);

		return result;
	}
}
