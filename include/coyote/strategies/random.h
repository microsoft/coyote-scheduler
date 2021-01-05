// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_RANDOM_H
#define COYOTE_RANDOM_H

#include <cstddef>
#include <cstdint>

namespace coyote
{
	// Implements the xoroshiro pseudorandom number generator.
	class Random
	{
	private:
		static constexpr unsigned STATE_BITS = 8 * sizeof(uint64_t);
		static constexpr unsigned RESULT_BITS = 8 * sizeof(uint64_t);

		uint64_t x;
		uint64_t y;

	public:
		Random(uint64_t seed) noexcept :
			x(seed == 0 ? 5489 : seed),
			y(seed)
		{
			next();
		}

		Random(Random&& strategy) = delete;
		Random(Random const&) = delete;

		Random& operator=(Random&& strategy) = delete;
		Random& operator=(Random const&) = delete;

		void seed(const uint64_t seed)
		{
			x = seed == 0 ? 5489 : seed;
			y = seed;
			next();
		}

		// Returns the next random number.
		uint64_t next()
		{
			uint64_t r = x + y;
			y ^= x;
			x = rotl(x, 55) ^ y ^ (y << 14);
			y = rotl(y, 36);
			return r >> (STATE_BITS - RESULT_BITS);
		}
	private:
		static inline uint64_t rotl(const uint64_t x, const uint64_t k)
		{
			return (x << k) | (x >> (STATE_BITS - k));
		}
	};
}

#endif // COYOTE_RANDOM_H
