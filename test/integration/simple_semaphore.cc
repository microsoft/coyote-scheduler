// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <thread>
#include <vector>
#include "test.h"

using namespace coyote;

constexpr auto SEMAPHORE_ID = 1;

Scheduler* scheduler;

int shared_var;
int max_value_observed;

// Mocked semaphore counter.
const int max_allowed = 2;
int current_acquired;

void mock_enter_semaphore()
{
	assert(current_acquired >= 0, "enter semaphore assertion failed");
	while (current_acquired == max_allowed)
	{
		scheduler->wait_resource(SEMAPHORE_ID);
	}

	current_acquired++;
}

void mock_exit_semaphore()
{
	assert(current_acquired <= max_allowed && current_acquired > 0, "exit semaphore assertion failed");
	current_acquired--;
	scheduler->signal_resource(SEMAPHORE_ID);
}

void work(int id)
{
	scheduler->start_operation(id);
	mock_enter_semaphore();

	shared_var++;
	if (shared_var > max_value_observed)
	{
		max_value_observed = shared_var;
	}

	scheduler->schedule_next();
	shared_var--;

	mock_exit_semaphore();
	scheduler->complete_operation(id);
}

void run_iteration()
{
	scheduler->attach();
	
	scheduler->create_resource(SEMAPHORE_ID);

	std::vector<std::unique_ptr<std::thread>> threads;
	for (int i = 0; i < 10; i++)
	{
		int thread_id = i + 1;
		scheduler->create_operation(thread_id);
		threads.push_back(std::make_unique<std::thread>(work, thread_id));
	}

	scheduler->schedule_next();
	assert(max_value_observed <= max_allowed, "the observed max value is greater than allowed");

	scheduler->detach();
	assert(scheduler->error_code(), ErrorCode::Success);

	for (int i = 0; i < 10; i++)
	{
		threads[i]->join();
	}
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
			max_value_observed = 0;
			current_acquired = 0;

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
