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

		// A strategy-specific bound.
		size_t strategy_bound;

		// The seed used by randomized strategies.
		uint64_t seed_state;

	public:
		Settings() noexcept;

		Settings(Settings&& strategy) = delete;
		Settings(Settings const&) = delete;

		Settings& operator=(Settings&& strategy) = delete;
		Settings& operator=(Settings const&) = delete;

		// Installs the random exploration strategy with the specified random seed.
		void use_random_strategy(uint64_t seed) noexcept;

		// Installs the PCT exploration strategy with the specified random seed and priority switch bound.
		void use_pct_strategy(uint64_t seed, size_t bound) noexcept;

		// Disables controlled scheduling.
		void disable_scheduling() noexcept;

		// Returns the type of the installed exploration strategy.
		StrategyType exploration_strategy() noexcept;

		// Returns an exploration strategy specific bound.
		size_t exploration_strategy_bound() noexcept;

		// Returns the seed used by randomized strategies.
		uint64_t random_seed() noexcept;
	};
}

#endif // COYOTE_CONFIGURATION_H
