// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pthread_model.cpp"
#include <vector>

using namespace coyote;

extern Scheduler* scheduler;
int shared_var;
int max_value_observed;
pthread_mutex_t mutex;

void* work(void* arg)
{
	FFI_pthread_mutex_lock(&mutex);

	shared_var++;
	if (shared_var > max_value_observed)
	{
		max_value_observed = shared_var;
	}

	scheduler->schedule_next();
	shared_var--;

	FFI_pthread_mutex_unlock(&mutex);

	return NULL;
}

void run_iteration()
{
	FFI_attach_scheduler();

	FFI_pthread_mutex_init(&mutex,NULL);
	pthread_t t1,t2;

    FFI_pthread_create(&t1,NULL,work,NULL);
    FFI_pthread_create(&t2,NULL,work,NULL);

	scheduler->schedule_next();

	FFI_pthread_join(t1,NULL);
    FFI_pthread_join(t2,NULL);
    FFI_pthread_mutex_destroy(&mutex);

    assert(max_value_observed == 1);

	FFI_detach_scheduler();
	assert(scheduler->error_code(), ErrorCode::Success);
}

int main()
{
	std::cout << "[test] started." << std::endl;
	auto start_time = std::chrono::steady_clock::now();

	try
	{
		scheduler = new Scheduler();

		for (int i = 0; i < 1000; i++)
		{
			// Initialize the state for the test iteration.
			shared_var = 0;
			max_value_observed = 0;

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