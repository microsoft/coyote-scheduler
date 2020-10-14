// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_SCHEDULER_H
#define COYOTE_SCHEDULER_H

#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_set>
#include "error_code.h"
#include "settings.h"
#include "operations/operation.h"
#include "operations/operations.h"
#include "strategies/random_strategy.h"

namespace coyote
{
	class Scheduler
	{
	private:
		// Configures the program exploration.
		std::unique_ptr<Settings> configuration;

		// Strategy for exploring the execution of the client program.
		std::unique_ptr<RandomStrategy> strategy;

		// Map from unique operation ids to operations.
		std::map<size_t, std::unique_ptr<Operation>> operation_map;

		// Vector of enabled and disabled operation ids.
		Operations operations;

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

		// The testing iteration count. It increments on each attach.
		size_t iteration_count;

		// The last assigned error code, else success.
		ErrorCode last_error_code;

	public:
		Scheduler() noexcept;
		Scheduler(std::unique_ptr<Settings> settings) noexcept;

		// Attaches to the scheduler. This should be called at the beginning of a testing iteration.
		// It creates a main operation with id '0'.
		ErrorCode attach() noexcept;

		// Detaches from the scheduler. This should be called at the end of a testing iteration.
		// It completes the main operation with id '0' and releases all controlled operations. 
		ErrorCode detach() noexcept;

		// Creates a new operation with the specified id.
		ErrorCode create_operation(size_t operation_id) noexcept;
		
		// Starts executing the operation with the specified id.
		ErrorCode start_operation(size_t operation_id) noexcept;

		// Waits until the operation with the specified id has completed.
		ErrorCode join_operation(size_t operation_id) noexcept;

		// Waits until the operations with the specified ids have completed.
		ErrorCode join_operations(const size_t* operation_ids, size_t size, bool wait_all) noexcept;

		// Completes executing the operation with the specified id and schedules the next operation.
		ErrorCode complete_operation(size_t operation_id) noexcept;

		// Creates a new resource with the specified id.
		ErrorCode create_resource(size_t resource_id) noexcept;

		// Waits the resource with the specified id to become available and schedules the next operation.
		ErrorCode wait_resource(size_t resource_id) noexcept;
		
		// Waits the resources with the specified ids to become available and schedules the next operation.
		ErrorCode wait_resources(const size_t* resource_ids, size_t size, bool wait_all) noexcept;
        
		// Signals all waiting operations that the resource with the specified id is available.
		ErrorCode signal_resource(size_t resource_id) noexcept;

		// Signals the waiting operation that the resource with the specified id is available.
		ErrorCode signal_resource(size_t resource_id, size_t operation_id) noexcept;

		// Deletes the resource with the specified id.
		ErrorCode delete_resource(size_t resource_id) noexcept;

		// Schedules the next operation, which can include the currently executing operation.
		// Only operations that are not blocked nor completed can be scheduled.
		ErrorCode schedule_next() noexcept;

		// Returns a controlled nondeterministic boolean value.
		bool next_boolean() noexcept;

		// Returns a controlled nondeterministic integer value chosen from the [0, max_value) range.
		size_t next_integer(size_t max_value) noexcept;

		// Returns the settings used by the scheduler.
		Settings* settings() noexcept;

		// Returns the last error code, if there is one assigned.
		ErrorCode error_code() noexcept;

	private:
		Scheduler(Scheduler&& op) = delete;
		Scheduler(Scheduler const&) = delete;

		Scheduler& operator=(Scheduler&& op) = delete;
		Scheduler& operator=(Scheduler const&) = delete;

		void create_operation_inner(size_t operation_id);
		void start_operation_inner(size_t operation_id, std::unique_lock<std::mutex>& lock);
		void schedule_next_inner(std::unique_lock<std::mutex>& lock);
	};
}

#endif // COYOTE_SCHEDULER_H
