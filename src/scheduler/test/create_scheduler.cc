// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <thread>
#include "test.h"

using namespace coyote;

int main()
{
	std::cout << "[test] started." << std::endl;

	try
	{
		Scheduler* scheduler = new Scheduler();
		scheduler->attach();
		assert(scheduler->error_code(), ErrorCode::Success);
		scheduler->detach();
		assert(scheduler->error_code(), ErrorCode::Success);
		delete scheduler;
	}
	catch (std::string error)
	{
		std::cout << "[test] failed: " << error << std::endl;
		return 1;
	}
	
	std::cout << "[test] done." << std::endl;
	return 0;
}
