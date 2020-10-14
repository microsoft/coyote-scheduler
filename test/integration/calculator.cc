// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <thread>
#include "test.h"

using namespace coyote;

constexpr auto WORK_THREAD_Add_ID = 1;
constexpr auto WORK_THREAD_Sub_ID = 2;
constexpr auto WORK_THREAD_Mul_ID = 3;
constexpr auto WORK_THREAD_Div_ID = 4;
constexpr auto WORK_THREAD_Res_ID = 5;

Scheduler* scheduler;

enum class CalculatorOp
{
	Add = 0,
	Sub = 1,
	Mul = 2,
	Div = 3,
	Res = 4
};

void work(int operation_id, CalculatorOp op)
{
	scheduler->start_operation(operation_id);

	for (int i = 1; i <= 10; i++)
	{
		if (op == CalculatorOp::Add)
		{
			std::cout << "Executing an 'Add' operation (#" << i << ")." << std::endl;
		}
		else if (op == CalculatorOp::Sub)
		{
			std::cout << "Executing a 'Sub' operation (#" << i << ")." << std::endl;
		}
		else if (op == CalculatorOp::Mul)
		{
			std::cout << "Executing a 'Mul' operation (#" << i << ")." << std::endl;
		}
		else if (op == CalculatorOp::Div)
		{
			std::cout << "Executing a 'Div' operation (#" << i << ")." << std::endl;
		}
		else if (op == CalculatorOp::Res)
		{
			std::cout << "Executing a 'Res' operation (#" << i << ")." << std::endl;
		}

		scheduler->schedule_next();
	}

	scheduler->complete_operation(operation_id);
}

void run_iteration()
{
	scheduler->attach();

	scheduler->create_operation(WORK_THREAD_Add_ID);
	std::thread addWorker(work, WORK_THREAD_Add_ID, CalculatorOp::Add);

	scheduler->create_operation(WORK_THREAD_Sub_ID);
	std::thread subWorker(work, WORK_THREAD_Sub_ID, CalculatorOp::Sub);

	scheduler->create_operation(WORK_THREAD_Mul_ID);
	std::thread mulWorker(work, WORK_THREAD_Mul_ID, CalculatorOp::Mul);

	scheduler->create_operation(WORK_THREAD_Div_ID);
	std::thread divWorker(work, WORK_THREAD_Div_ID, CalculatorOp::Div);

	scheduler->create_operation(WORK_THREAD_Res_ID);
	std::thread resWorker(work, WORK_THREAD_Res_ID, CalculatorOp::Res);

	scheduler->schedule_next();

	scheduler->join_operation(WORK_THREAD_Add_ID);
	scheduler->join_operation(WORK_THREAD_Sub_ID);
	scheduler->join_operation(WORK_THREAD_Mul_ID);
	scheduler->join_operation(WORK_THREAD_Div_ID);
	scheduler->join_operation(WORK_THREAD_Res_ID);
	addWorker.join();
	subWorker.join();
	mulWorker.join();
	divWorker.join();
	resWorker.join();

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

		for (int i = 0; i < 2; i++)
		{
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
