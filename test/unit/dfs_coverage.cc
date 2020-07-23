// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <set>
#include <thread>
#include "test.h"

using namespace coyote;

constexpr auto WORK_THREAD_1_ID = 1;
constexpr auto WORK_THREAD_2_ID = 2;
constexpr auto WORK_THREAD_3_ID = 3;
constexpr auto WORK_THREAD_4_ID = 4;
constexpr auto WORK_THREAD_5_ID = 5;

Scheduler* scheduler;

std::string curr_trace;
std::set<std::string> trace_all;

void work_1()
{
	scheduler->start_operation(WORK_THREAD_1_ID);
	curr_trace += "1";
	scheduler->complete_operation(WORK_THREAD_1_ID);
}

void work_2()
{
	scheduler->start_operation(WORK_THREAD_2_ID);
	curr_trace += "2";
	scheduler->complete_operation(WORK_THREAD_2_ID);
}

void work_3()
{
	scheduler->start_operation(WORK_THREAD_3_ID);
	curr_trace += "3";
	scheduler->complete_operation(WORK_THREAD_3_ID);
}

void work_4()
{
	scheduler->start_operation(WORK_THREAD_4_ID);
	curr_trace += "4";
	scheduler->complete_operation(WORK_THREAD_4_ID);
}

void work_5()
{
	scheduler->start_operation(WORK_THREAD_5_ID);
	curr_trace += "5";
	scheduler->complete_operation(WORK_THREAD_5_ID);
}


void run_iteration()
{
	scheduler->attach();

	scheduler->create_operation(WORK_THREAD_1_ID);
	std::thread t1(work_1);

	scheduler->create_operation(WORK_THREAD_2_ID);
	std::thread t2(work_2);

	scheduler->create_operation(WORK_THREAD_3_ID);
	std::thread t3(work_3);

	scheduler->create_operation(WORK_THREAD_4_ID);
	std::thread t4(work_4);

	scheduler->create_operation(WORK_THREAD_5_ID);
	std::thread t5(work_5);

	scheduler->schedule_next();

	scheduler->join_operation(WORK_THREAD_1_ID);
	scheduler->join_operation(WORK_THREAD_2_ID);
	scheduler->join_operation(WORK_THREAD_3_ID);
	scheduler->join_operation(WORK_THREAD_4_ID);
	scheduler->join_operation(WORK_THREAD_5_ID);

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();

	trace_all.insert(curr_trace);

	scheduler->detach();
	assert(scheduler->error_code(), ErrorCode::Success);
}

// This unit-test is to check all possible combinations of thread(s) execution explored by DFS Strategy.
// Five threads (with id's 1-5) are used for testing. Each thread append it's id to the trace during execution. 
// Each unique traces are added to 'trace_all'. 120 uniques traces should be explored by DFS Strategy.
// At end of all iteration, 'trace_all.size() == 120' denotes all paths explored by DFS Strategy.
int main()
{
	std::cout << "[test] started." << std::endl;

	try
	{
		scheduler = new Scheduler("DFSStrategy");

		for (int i = 0; i < 1000; i++)
		{
#ifdef COYOTE_DEBUG_LOG
			std::cout << "[test] iteration " << i << std::endl;
#endif // COYOTE_DEBUG_LOG
			curr_trace = "";
			run_iteration();
		}

		delete scheduler;

		assert(trace_all.size() == 120, "All execution paths not covered by DFS testing.");
	}
	catch (std::string error)
	{
		std::cout << "[test] failed: " << error << std::endl;
		return 1;
	}

	return 0;
}
