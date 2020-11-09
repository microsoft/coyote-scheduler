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

	void Settings::use_random_strategy(uint64_t seed)
	{
		strategy_type = StrategyType::Random;
		seed_state = seed;
	}

	void Settings::disable_scheduling()
	{
		strategy_type = StrategyType::None;
	}

	StrategyType Settings::exploration_strategy()
	{
		return strategy_type;
	}

	uint64_t Settings::random_seed()
	{
		return seed_state;
	}
}
