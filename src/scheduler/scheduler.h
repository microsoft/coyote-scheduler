// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_set>
#include "errors/error_code.h"
#include "operations/operation.h"
#include "strategies/random_strategy.h"

namespace coyote
{
	class Scheduler
	{
	private:
		// Strategy for exploring the execution of the client program.
		std::unique_ptr<RandomStrategy> strategy;

		// Map from unique operation ids to operations.
		std::map<size_t, std::shared_ptr<Operation>> operation_map;

		// Map from unique resource ids to blocked operation ids.
		std::map<size_t, std::shared_ptr<std::unordered_set<size_t>>> resource_map;

		// Mutex that synchronizes access to the scheduler.
		std::unique_ptr<std::mutex> mutex;

		// Conditional variable that can be used to block scheduling a next operation until all pending
		// operations have started.
		std::condition_variable pending_operations_cv;

		// The id of the currently scheduled operation.
		size_t scheduled_operation_id;

		// Count of newly created operations that have not started yet.
		size_t pending_start_operation_count;

		// The id of the main operation.
		const size_t main_operation_id = 0;

		// True if an execution is attached to the scheduler, else false.
		bool is_attached;

		// The last assigned error code, else success.
		std::error_code last_error_code;

	public:
		Scheduler() noexcept;
		Scheduler(size_t seed) noexcept;

		// Attaches to the scheduler. This should be called at the beginning of a testing iteration.
		// It creates a main operation with id '0'.
		std::error_code attach() noexcept;

		// Detaches from the scheduler. This should be called at the end of a testing iteration.
		// It completes the main operation with id '0' and releases all controlled operations. 
		std::error_code detach() noexcept;

		// Creates a new operation with the specified id.
		std::error_code create_operation(size_t operation_id) noexcept;
		
		// Starts executing the operation with the specified id.
		std::error_code start_operation(size_t operation_id) noexcept;

		// Waits until the operation with the specified id has completed.
		std::error_code join_operation(size_t operation_id) noexcept;

		// Waits until the operations with the specified ids have completed.
		std::error_code join_operations(const size_t* operation_ids, int size, bool wait_all) noexcept;

		// Completes executing the operation with the specified id and schedules the next operation.
		std::error_code complete_operation(size_t operation_id) noexcept;

		// Creates a new resource with the specified id.
		std::error_code create_resource(size_t resource_id) noexcept;

		// Waits the resource with the specified id to become available and schedules the next operation.
		std::error_code wait_resource(size_t resource_id) noexcept;
		
		// Waits the resources with the specified ids to become available and schedules the next operation.
		std::error_code wait_resources(const size_t* resource_ids, int size, bool wait_all) noexcept;
        
		// Signals the resource with the specified id is available.
		std::error_code signal_resource(size_t resource_id) noexcept;

		// Deletes the resource with the specified id.
		std::error_code delete_resource(size_t resource_id) noexcept;

		// Schedules the next operation, which can include the currently executing operation.
		// Only operations that are not blocked nor completed can be scheduled.
		std::error_code schedule_next() noexcept;

		// Returns a controlled nondeterministic boolean value.
		bool next_boolean() noexcept;

		// Returns a controlled nondeterministic integer value chosen from the [0, max_value) range.
		int next_integer(int max_value) noexcept;

		// Returns a seed that can be used to reproduce the current testing iteration.
		size_t seed() noexcept;

		// Returns the last error code, if there is one assigned.
		std::error_code error_code() noexcept;

	private:
		Scheduler(Scheduler&& op) = delete;
		Scheduler(Scheduler const&) = delete;

		Scheduler& operator=(Scheduler&& op) = delete;
		Scheduler& operator=(Scheduler const&) = delete;

		void create_operation(size_t operation_id, std::unique_lock<std::mutex>& lock);
		void start_operation(size_t operation_id, std::unique_lock<std::mutex>& lock);
		void schedule_next(std::unique_lock<std::mutex>& lock);

		const std::vector<std::shared_ptr<Operation>> enabled_operations();
	};
}

#endif // SCHEDULER_H
