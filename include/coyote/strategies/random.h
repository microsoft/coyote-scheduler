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
		static constexpr unsigned BITS = 8 * sizeof(size_t);

		size_t state_x;
		size_t state_y;

	public:
		Random(size_t seed) noexcept;

		Random(Random&& strategy) = delete;
		Random(Random const&) = delete;

		Random& operator=(Random&& strategy) = delete;
		Random& operator=(Random const&) = delete;

		void seed(const size_t seed);

		// Returns the next random number.
		size_t next();

	private:
		static inline size_t rotl(const size_t x, const size_t k)
		{
			return (x << k) | (x >> (BITS - k));
		}
	};
}

#endif // COYOTE_RANDOM_H
