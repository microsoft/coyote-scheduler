// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <set>
#include <thread>
#include "test.h"

using namespace coyote;

constexpr auto WORK_THREAD_1_ID = 1;
constexpr auto WORK_THREAD_2_ID = 2;

Scheduler* scheduler;

std::string curr_trace;
std::multiset<std::string> trace_all;

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
	scheduler->schedule_next();
	std::string choice = "[" + std::to_string(scheduler->next_integer(4)) + "]";
	curr_trace += choice;
	scheduler->complete_operation(WORK_THREAD_2_ID);
}

void run_iteration()
{
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

	auto set_it = trace_all.find(curr_trace);
	if (set_it != trace_all.end())
	{
		trace_all.erase(set_it);
	}

	scheduler->detach();
	assert(scheduler->error_code(), ErrorCode::Success);
}

// This unit-test is to check all possible combinations of integer choices explored by DFS Strategy.
// Two threads (with id's 1 and 2) and a next_integer(4) choice are used for testing.
// 12 uniques traces should be explored by DFS Strategy. 'trace_all' contains all unique combinations of thread and
// integer choices to be explored by DFS Strategy. For example "12[0]" represents '1' & '2' as thread id's and [0]
// denotes the integer choice made by DFS Strategy. On each iteration, unit-test generates a unique trace and it is removed
// from 'trace_all'. At end of all iteration, empty 'trace_all' denotes all paths explored by DFS Strategy.
int main()
{
	std::cout << "[test] started." << std::endl;

	trace_all.insert("12[0]");
	trace_all.insert("12[1]");
	trace_all.insert("12[2]");
	trace_all.insert("12[3]");
	trace_all.insert("21[0]");
	trace_all.insert("21[1]");
	trace_all.insert("21[2]");
	trace_all.insert("21[3]");
	trace_all.insert("2[0]1");
	trace_all.insert("2[1]1");
	trace_all.insert("2[2]1");
	trace_all.insert("2[3]1");
	

	try
	{
		scheduler = new Scheduler("DFSStrategy");

		for (int i = 0; i < 50; i++)
		{
#ifdef COYOTE_DEBUG_LOG
			std::cout << "[test] iteration " << i << std::endl;
#endif // COYOTE_DEBUG_LOG
			curr_trace = "";
			run_iteration();
		}

		delete scheduler;

		assert(trace_all.size() == 0, "All execution paths not covered by DFS testing.");
	}
	catch (std::string error)
	{
		std::cout << "[test] failed: " << error << std::endl;
		return 1;
	}

	return 0;
}
