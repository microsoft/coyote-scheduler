// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_CONFIGURATION_H
#define COYOTE_CONFIGURATION_H

#include "strategies/strategy.h"

namespace coyote
{
	class Settings
	{
	private:
		// The execution exploration strategy.
		Strategy strategy;

		// The seed used by randomized strategies.
		size_t seed_value;

	public:
		Settings() noexcept;

		Settings(Settings&& strategy) = delete;
		Settings(Settings const&) = delete;

		Settings& operator=(Settings&& strategy) = delete;
		Settings& operator=(Settings const&) = delete;

		// Installs the random exploration strategy with the specified random seed.
		void use_random_strategy(size_t seed);

		// Returns the seed used by randomized strategies.
		size_t seed();
	};
}

#endif // COYOTE_CONFIGURATION_H
