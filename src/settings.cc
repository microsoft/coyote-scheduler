// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <chrono>
#include <stdexcept>
#include "settings.h"

namespace coyote
{
	Settings::Settings() noexcept :
		strategy_type(StrategyType::Random),
		seed_state(std::chrono::high_resolution_clock::now().time_since_epoch().count())
	{
	}

	void Settings::use_random_strategy(uint64_t seed) noexcept
	{
		strategy_type = StrategyType::Random;
		seed_state = seed;
		strategy_bound = 100;
	}

	void Settings::use_random_strategy(uint64_t seed, size_t probability)
	{
		if ( probability > 100) {
        throw std::invalid_argument("received probability greater than 100");
    }

		strategy_type = StrategyType::Random;
		seed_state = seed;
		strategy_bound = probability;
	}

	void Settings::use_pct_strategy(uint64_t seed, size_t bound) noexcept
	{
		strategy_type = StrategyType::PCT;
		seed_state = seed;
		strategy_bound = bound;
	}

	void Settings::disable_scheduling() noexcept
	{
		strategy_type = StrategyType::None;
	}

	StrategyType Settings::exploration_strategy() noexcept
	{
		return strategy_type;
	}

	size_t Settings::exploration_strategy_bound() noexcept
	{
		return strategy_bound;
	}

	uint64_t Settings::random_seed() noexcept
	{
		return seed_state;
	}
}
