// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pthread_model.cpp"
#include <vector>

using namespace coyote;

extern Scheduler* scheduler;

int arr[10], flag = 0;
pthread_mutex_t fill_mutex;
pthread_cond_t cond_var;
bool is_waiting;

// Thread 1 will initilaize the array
void* fill(void* arg)
{
	for(int i = 0; i < 10; i++) {
		arr[i] = i;
	}

	// Loop until someone waits on the conditional variable.
	while(!is_waiting)
	{
		scheduler->schedule_next();
	}

	FFI_pthread_mutex_lock(&fill_mutex);
	FFI_pthread_cond_signal(&cond_var);
	FFI_pthread_mutex_unlock(&fill_mutex);

	return NULL;
}

// Thread 2 will increment the array
void* increment(void* arg)
{
	FFI_pthread_mutex_lock(&fill_mutex);
	is_waiting = true;
	FFI_pthread_cond_wait(&cond_var, &fill_mutex);
	FFI_pthread_mutex_unlock(&fill_mutex);

	for(int i = 0; i < 10; i++) {
		arr[i]++;
	}

	return NULL;
}

void run_iteration()
{
	FFI_attach_scheduler();

	FFI_pthread_mutex_init(&fill_mutex, NULL);
	FFI_pthread_cond_init(&cond_var, NULL);

	pthread_t t1,t2;

	FFI_pthread_create(&t1, NULL, &fill, NULL);
	FFI_pthread_create(&t2, NULL, &increment, NULL);

	scheduler->schedule_next();

	FFI_pthread_join(t1, NULL);
	FFI_pthread_join(t2, NULL);

	FFI_pthread_cond_destroy(&cond_var);
	FFI_pthread_mutex_destroy(&fill_mutex);

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

		for (int j = 0; j < 1000; j++)
		{
			// Initialize the state for the test iteration.
			for(int i = 0; i < 10; i++) {
				arr[i] = 0;
			}
			is_waiting = false;

#ifdef COYOTE_DEBUG_LOG
			std::cout << "[test] iteration " << j << std::endl;
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