// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <thread>
#include "test.h"

using namespace coyote;

constexpr auto WORK_THREAD_1_ID = 1;
constexpr auto WORK_THREAD_2_ID = 2;

Scheduler* scheduler;

int shared_var;
bool race_found;
size_t race_seed;

void work_1()
{
	scheduler->start_operation(WORK_THREAD_1_ID);

	shared_var = 1;
	scheduler->schedule_next();
	if (shared_var != 1)
	{
		std::cout << "[test] found race condition in thread 1." << std::endl;
		race_found = true;
	}

	scheduler->complete_operation(WORK_THREAD_1_ID);
}

void work_2()
{
	scheduler->start_operation(WORK_THREAD_2_ID);

	shared_var = 2;
	scheduler->schedule_next();
	if (shared_var != 2)
	{
		std::cout << "[test] found race condition in thread 2." << std::endl;
		race_found = true;
	}

	scheduler->complete_operation(WORK_THREAD_2_ID);
}

void run_iteration()
{
	shared_var = 0;
	race_found = false;

	scheduler->attach();

	scheduler->create_operation(WORK_THREAD_1_ID);
	std::thread t1(work_1);

	scheduler->create_operation(WORK_THREAD_2_ID);
	std::thread t2(work_2);

	scheduler->schedule_next();

	scheduler->join_operation(WORK_THREAD_1_ID);
	scheduler->join_operation(WORK_THREAD_2_ID);
	t1.join();
	t2.join();

	scheduler->detach();
	assert(scheduler->error_code(), ErrorCode::Success);
}

void test()
{
	scheduler = new Scheduler();

	for (int i = 0; i < 100; i++)
	{
		std::cout << "[test] iteration " << i << std::endl;
		run_iteration();
		if (race_found)
		{
			race_seed = scheduler->seed();
			break;
		}
	}

	assert(race_found, "race was not found.");
	delete scheduler;
}

void replay()
{
	scheduler = new Scheduler(race_seed);

	std::cout << "[test] replaying using seed " << race_seed << std::endl;
	run_iteration();

	assert(race_found, "race was not found.");
	delete scheduler;
}

int main()
{
	std::cout << "[test] started." << std::endl;

	try
	{
		// Try to find the race condition.
		test();

		// Try to replay the bug.
		replay();
	}
	catch (std::string error)
	{
		std::cout << "[test] failed: " << error << std::endl;
		return 1;
	}

	std::cout << "[test] done." << std::endl;
	return 0;
}
