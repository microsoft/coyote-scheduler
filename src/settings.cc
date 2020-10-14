// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <chrono>
#include "settings.h"

namespace coyote
{
	Settings::Settings() noexcept :
		strategy(Strategy::Random),
		seed_state(std::chrono::high_resolution_clock::now().time_since_epoch().count())
	{
	}

	void Settings::use_random_strategy(size_t seed)
	{
		strategy = Strategy::Random;
		seed_state = seed;
	}

	void Settings::disable_scheduling()
	{
		strategy = Strategy::None;
	}

	Strategy Settings::exploration_strategy()
	{
		return strategy;
	}

	size_t Settings::random_seed()
	{
		return seed_state;
	}
}
