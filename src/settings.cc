// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <chrono>
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
	}

	void Settings::use_probabilistic_strategy(uint64_t seed, size_t difficulty) noexcept
	{
		strategy_type = StrategyType::Probabilistic;
		seed_state = seed;
		strategy_bound = difficulty;
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
