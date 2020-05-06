// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <thread>
#include "test.h"

using namespace coyote;

constexpr auto RESOURCE_ID = 1;
constexpr auto WORK_THREAD_ID = 1;

Scheduler* scheduler;

int shared_var;
bool work_completed;

void mock_join()
{
	while (!work_completed)
	{
		scheduler->wait_resource(RESOURCE_ID);
	}
}

void work()
{
	scheduler->start_operation(WORK_THREAD_ID);
	shared_var = 1;
	work_completed = true;
	scheduler->signal_resource(RESOURCE_ID);
	scheduler->complete_operation(WORK_THREAD_ID);
}

void run_iteration()
{
	scheduler->attach();

	scheduler->create_operation(WORK_THREAD_ID);
	scheduler->create_resource(RESOURCE_ID);
	std::thread t(work);

	mock_join();
	t.join();

	assert(shared_var == 1, "the shared variable is not equal to 1");

	scheduler->detach();
	assert(scheduler->error_code(), ErrorCode::Success);
}

int main()
{
	std::cout << "[test] started." << std::endl;
	auto start_time = std::chrono::steady_clock::now();

	try
	{
		scheduler = new Scheduler();

		for (int i = 0; i < 100; i++)
		{
			// Initialize the state for the test iteration.
			shared_var = 0;
			work_completed = false;

#ifdef COYOTE_DEBUG_LOG
			std::cout << "[test] iteration " << i << std::endl;
#endif // COYOTE_DEBUG_LOG
			run_iteration();
		}

		delete scheduler;
	}
	catch (std::string error)
	{
		std::cout << "[test] failed: " << error << std::endl;
		return 1;
	}

	std::cout << "[test] done in " << total_time(start_time) << "ms." << std::endl;
	return 0;
}
