// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <thread>
#include "test.h"

using namespace coyote;

constexpr auto WORK_THREAD_ID = 1;

Scheduler* scheduler;

int shared_var = 0;
int thread_completed = 0;

void mock_join()
{
	while (thread_completed == 0) 
	{
		scheduler->wait_resource(WORK_THREAD_ID);
	}
}

void work()
{
	scheduler->start_operation(WORK_THREAD_ID);
	shared_var = 1;
	thread_completed = 1;
	scheduler->signal_resource(WORK_THREAD_ID);
	scheduler->complete_operation(WORK_THREAD_ID);
}

void run_iteration()
{
	scheduler->attach();

	scheduler->create_operation(WORK_THREAD_ID);
	scheduler->create_resource(WORK_THREAD_ID);
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

	try
	{
		scheduler = new Scheduler();

		for (int i = 0; i < 100; i++)
		{
			std::cout << "[test] iteration " << i << std::endl;
			run_iteration();

			shared_var = 0;
			thread_completed = 0;
		}

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
