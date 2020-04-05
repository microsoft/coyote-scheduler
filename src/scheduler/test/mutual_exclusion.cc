// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <thread>
#include "test.h"

using namespace coyote;

constexpr auto WORK_THREAD_1_ID = 1;
constexpr auto WORK_THREAD_2_ID = 2;
constexpr auto LOCK_ID = 1;

Scheduler* scheduler;

int shared_var = 0;
int lock_status = 0;

void mock_acquire()
{
	scheduler->schedule_next();

	do
	{
		if (lock_status == 0)
		{
			lock_status = 1;
			break;
		}
		else
		{
			scheduler->wait_resource(LOCK_ID);
		}
	} while (true);
}

void mock_release()
{
	scheduler->schedule_next();
	assert(lock_status == 1, "lock status is not 1.");
	lock_status = 0;
	scheduler->signal_resource(LOCK_ID);
}

void work_1()
{
	scheduler->start_operation(WORK_THREAD_1_ID);

	mock_acquire();
	shared_var = 1;
	scheduler->schedule_next();
	assert(shared_var == 1, "shared variable is not 1.");
	mock_release();

	scheduler->complete_operation(WORK_THREAD_1_ID);
}

void work_2()
{
	scheduler->start_operation(WORK_THREAD_2_ID);

	mock_acquire();
	shared_var = 2;
	scheduler->schedule_next();
	assert(shared_var == 2, "shared variable is not 2.");
	mock_release();

	scheduler->complete_operation(WORK_THREAD_2_ID);
}

void run_iteration()
{
	scheduler->attach();

	scheduler->create_resource(LOCK_ID);
	scheduler->create_operation(WORK_THREAD_1_ID);
	std::thread t1(work_1);

	scheduler->create_operation(WORK_THREAD_2_ID);
	std::thread t2(work_2);

	scheduler->join_operation(WORK_THREAD_1_ID);
	scheduler->join_operation(WORK_THREAD_2_ID);
	t1.join();
	t2.join();

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
			lock_status = 0;
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
