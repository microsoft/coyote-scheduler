// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "test.h"

using namespace coyote;

constexpr auto WORK_THREAD_1_ID = 1;
constexpr auto WORK_THREAD_2_ID = 2;

Scheduler* scheduler;

int shared_var = 0;
bool race_found = false;

void work_1()
{
	scheduler->start_operation(WORK_THREAD_1_ID);

	shared_var = 1;
	scheduler->schedule_next_operation();
	if (shared_var != 1)
	{
		race_found = true;
	}

	scheduler->complete_operation(WORK_THREAD_1_ID);
}

void work_2()
{
	scheduler->start_operation(WORK_THREAD_2_ID);

	shared_var = 2;
	scheduler->schedule_next_operation();
	if (shared_var != 2)
	{
		race_found = true;
	}

	scheduler->complete_operation(WORK_THREAD_2_ID);
}

void run_iteration()
{
	scheduler->attach();

	scheduler->create_operation(WORK_THREAD_1_ID);
	std::thread t1(work_1);

	scheduler->create_operation(WORK_THREAD_2_ID);
	std::thread t2(work_2);

	scheduler->schedule_next_operation();

	scheduler->join_operation(WORK_THREAD_1_ID);
	scheduler->join_operation(WORK_THREAD_2_ID);
	t1.join();
	t2.join();

	scheduler->detach();
	assert(scheduler->get_last_error_code(), ErrorCode::Success);
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
		}

		assert(race_found, "race was not found.");
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
