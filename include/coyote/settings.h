// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_CONFIGURATION_H
#define COYOTE_CONFIGURATION_H

#include "strategies/strategy_type.h"

namespace coyote
{
	class Settings
	{
	private:
		// The execution exploration strategy.
		StrategyType strategy_type;

		// The seed used by randomized strategies.
		uint64_t seed_state;

	public:
		Settings() noexcept;

		Settings(Settings&& strategy) = delete;
		Settings(Settings const&) = delete;

		Settings& operator=(Settings&& strategy) = delete;
		Settings& operator=(Settings const&) = delete;

		// Installs the random exploration strategy with the specified random seed.
		void use_random_strategy(uint64_t seed);

		// Disables controlled scheduling.
		void disable_scheduling();

		// Returns the type of the installed exploration strategy.
		StrategyType exploration_strategy();

		// Returns the seed used by randomized strategies.
		uint64_t random_seed();
	};
}

#endif // COYOTE_CONFIGURATION_H
