// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <cstdint>
#include "strategies/random.h"

namespace coyote
{
	Random::Random(uint64_t seed) noexcept :
		x(seed == 0 ? 5489 : seed),
		y(seed)
	{
		next();
	}

	void Random::seed(const uint64_t seed)
	{
		x = seed == 0 ? 5489 : seed;
		y = seed;
		next();
	}

	uint64_t Random::next()
	{
		uint64_t r = x + y;
		y ^= x;
		x = rotl(x, 55) ^ y ^ (y << 14);
		y = rotl(y, 36);
		return r >> (STATE_BITS - RESULT_BITS);
	}
}
