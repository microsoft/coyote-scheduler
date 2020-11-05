// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_RANDOM_H
#define COYOTE_RANDOM_H

#include <cstddef>

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
		Random(uint64_t seed) noexcept;

		Random(Random&& strategy) = delete;
		Random(Random const&) = delete;

		Random& operator=(Random&& strategy) = delete;
		Random& operator=(Random const&) = delete;

		void seed(const uint64_t seed);

		// Returns the next random number.
        uint64_t next();

	private:
		static inline uint64_t rotl(const uint64_t x, const uint64_t k)
		{
			return (x << k) | (x >> (STATE_BITS - k));
		}
	};
}

#endif // COYOTE_RANDOM_H
