// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <thread>
#include <vector>
#include "test.h"

using namespace coyote;

constexpr auto RESOURCE_ID_1 = 1;
constexpr auto RESOURCE_ID_2 = 2;
constexpr auto WORK_THREAD_1_ID = 1;
constexpr auto WORK_THREAD_2_ID = 2;
constexpr auto WORK_THREAD_3_ID = 3;

Scheduler* scheduler;

bool work_1_completed;
bool work_2_completed;

void work(int operation_id)
{
	scheduler->start_operation(operation_id);

	while (!work_1_completed)
	{
		scheduler->wait_resource(RESOURCE_ID_1);
	}

	scheduler->schedule_next();

	while (!work_2_completed)
	{
		scheduler->wait_resource(RESOURCE_ID_2);
	}

	scheduler->complete_operation(operation_id);
}

void run_iteration()
{
	scheduler->attach();

	scheduler->create_resource(RESOURCE_ID_1);
	scheduler->create_resource(RESOURCE_ID_2);

	scheduler->create_operation(WORK_THREAD_1_ID);
	std::thread t1(work, WORK_THREAD_1_ID);

	scheduler->schedule_next();

	work_1_completed = true;
	scheduler->signal_resource(RESOURCE_ID_1, WORK_THREAD_1_ID);

	scheduler->schedule_next();

	work_2_completed = true;
	scheduler->signal_resource(RESOURCE_ID_2, WORK_THREAD_1_ID);

	scheduler->join_operation(WORK_THREAD_1_ID);
	t1.join();

	work_1_completed = false;
	work_2_completed = false;

	scheduler->create_operation(WORK_THREAD_2_ID);
	std::thread t2(work, WORK_THREAD_2_ID);

	scheduler->create_operation(WORK_THREAD_3_ID);
	std::thread t3(work, WORK_THREAD_3_ID);

	scheduler->schedule_next();

	work_1_completed = true;
	scheduler->signal_resource(RESOURCE_ID_1);

	scheduler->schedule_next();

	work_2_completed = true;
	scheduler->signal_resource(RESOURCE_ID_2);

	scheduler->join_operation(WORK_THREAD_2_ID);
	scheduler->join_operation(WORK_THREAD_3_ID);
	t2.join();
	t3.join();

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
			work_1_completed = false;
			work_2_completed = false;

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
