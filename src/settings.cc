// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <chrono>
#include "settings.h"

namespace coyote
{
	Settings::Settings() noexcept :
		strategy(Strategy::Random),
		seed_value(std::chrono::high_resolution_clock::now().time_since_epoch().count())
	{
	}

	void Settings::use_random_strategy(size_t seed)
	{
		strategy = Strategy::Random;
		seed_value = seed;
	}

	size_t Settings::seed()
	{
		return seed_value;
	}
}
