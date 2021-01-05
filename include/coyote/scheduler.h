// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_SCHEDULER_H
#define COYOTE_SCHEDULER_H

#include <iostream>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>
#include "error_code.h"
#include "settings.h"
#include "operations/operation.h"
#include "operations/operations.h"
#include "operations/operation_status.h"
#include "strategies/strategy.h"
#include "strategies/random_strategy.h"
#include "strategies/pct_strategy.h"

namespace coyote
{
	class Scheduler
	{
	private:
		// Configures the program exploration.
		std::unique_ptr<Settings> configuration;

		// Strategy for exploring the execution of the client program.
		std::unique_ptr<Strategy> strategy;

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
		size_t scheduled_op_id;

		// Count of newly created operations that have not started yet.
		size_t pending_start_operation_count;

		// The id of the main operation.
		const size_t main_op_id = 0;

		// True if an execution is attached to the scheduler, else false.
		bool is_attached;

		// The testing iteration count. It increments on each attach.
		size_t iteration_count;

		// The last assigned error code, else success.
		ErrorCode last_error_code;

	public:
		Scheduler() noexcept :
			Scheduler(std::make_unique<Settings>())
		{
		}
		
		Scheduler(std::unique_ptr<Settings> settings) noexcept :
			configuration(std::move(settings)),
			strategy(create_strategy()),
			mutex(std::make_unique<std::mutex>()),
			pending_operations_cv(),
			scheduled_op_id(0),
			pending_start_operation_count(0),
			is_attached(false),
			iteration_count(0),
			last_error_code(ErrorCode::Success)
		{
		}

		// Attaches to the scheduler. This should be called at the beginning of a testing iteration.
		// It creates a main operation with id '0'.
		ErrorCode attach() noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::attach] attaching the main operation" << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (is_attached)
				{
					throw ErrorCode::ClientAttached;
				}

				is_attached = true;
				iteration_count += 1;
				last_error_code = ErrorCode::Success;

				if (iteration_count > 1)
				{
					// Prepare the strategy for the next iteration.
					strategy->prepare_next_iteration(iteration_count);
				}

				create_operation_inner(main_op_id);
				start_operation_inner(main_op_id, lock);
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Detaches from the scheduler. This should be called at the end of a testing iteration.
		// It completes the main operation with id '0' and releases all controlled operations. 
		ErrorCode detach() noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::detach] releasing all operations" << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				is_attached = false;

				Operation* main_op = operation_map.at(main_op_id).get();
				main_op->status = OperationStatus::Completed;
				operations.disable(main_op->id);

				for (auto& kvp : operation_map)
				{
					Operation* next_op = kvp.second.get();
					if (next_op->status != OperationStatus::Completed)
					{
	#ifdef COYOTE_DEBUG_LOG
						std::cout << "[coyote::detach] canceling operation " << next_op->id << std::endl;
	#endif // COYOTE_DEBUG_LOG
						// If the operation has not already completed, then cancel it.
						next_op->is_scheduled = true;
						next_op->status = OperationStatus::Completed;
						operations.disable(next_op->id);
						next_op->cv.notify_all();
					}
				}

				operation_map.clear();
				operations.clear();
				resource_map.clear();
				pending_start_operation_count = 0;
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Creates a new operation with the specified id.
		ErrorCode create_operation(size_t operation_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::create_operation] creating operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}
				else if (operation_id == main_op_id)
				{
					throw ErrorCode::MainOperationExplicitlyCreated;
				}

				create_operation_inner(operation_id);
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}
		
		// Starts executing the operation with the specified id.
		ErrorCode start_operation(size_t operation_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::start_operation] starting operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}
				else if (operation_id == main_op_id)
				{
					throw ErrorCode::MainOperationExplicitlyStarted;
				}

				start_operation_inner(operation_id, lock);
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Waits until the operation with the specified id has completed.
		ErrorCode join_operation(size_t operation_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::join_operation] joining operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				auto it = operation_map.find(operation_id);
				if (it == operation_map.end())
				{
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::join_operation] not existing operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG
					throw ErrorCode::NotExistingOperation;
				}

				Operation* join_op = it->second.get();
				if (join_op->status != OperationStatus::Completed)
				{
					join_op->blocked_operation_ids.insert(scheduled_op_id);

					Operation* scheduled_op = operation_map.at(scheduled_op_id).get();
					scheduled_op->join_operation(operation_id);
					operations.disable(scheduled_op->id);

					// Waiting for the resource to be released, so schedule the next enabled operation.
					schedule_next_inner(lock);
				}
	#ifdef COYOTE_DEBUG_LOG
				else
				{
					std::cout << "[coyote::join_operation] already completed operation " << operation_id << std::endl;
				}
	#endif // COYOTE_DEBUG_LOG
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Waits until the operations with the specified ids have completed.
		ErrorCode join_operations(const size_t* operation_ids, size_t size, bool wait_all) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
				if (wait_all)
				{
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::join_operations] joining all " << size << " operations" << std::endl;
	#endif // COYOTE_DEBUG_LOG
				}
				else
				{
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::join_operations] joining any of " << size << " operations" << std::endl;
	#endif // COYOTE_DEBUG_LOG
				}

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				std::vector<size_t> join_operations;
				for (size_t i = 0; i < size; i++)
				{
					size_t operation_id = *(operation_ids + i);
					auto it = operation_map.find(operation_id);
					if (it == operation_map.end())
					{
	#ifdef COYOTE_DEBUG_LOG
						std::cout << "[coyote::join_operations] not existing operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG
						throw ErrorCode::NotExistingOperation;
					}

					Operation* join_op = it->second.get();
					if (join_op->status != OperationStatus::Completed)
					{
						join_op->blocked_operation_ids.insert(scheduled_op_id);
						join_operations.push_back(operation_id);
					}
	#ifdef COYOTE_DEBUG_LOG
					else
					{
						std::cout << "[coyote::join_operation] already completed operation " << operation_id << std::endl;
					}
	#endif // COYOTE_DEBUG_LOG
				}

				if (!join_operations.empty())
				{
					Operation* scheduled_op = operation_map.at(scheduled_op_id).get();
					scheduled_op->join_operations(join_operations, wait_all);
					operations.disable(scheduled_op->id);

					// Waiting for the resources to be released, so schedule the next enabled operation.
					schedule_next_inner(lock);
				}
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Completes executing the operation with the specified id and schedules the next operation.
		ErrorCode complete_operation(size_t operation_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::complete_operation] completing operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}
				else if (operation_id == main_op_id)
				{
					throw ErrorCode::MainOperationExplicitlyCompleted;
				}

				auto it = operation_map.find(operation_id);
				if (it == operation_map.end())
				{
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::complete_operation] not existing operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG
					throw ErrorCode::NotExistingOperation;
				}

				Operation* op = it->second.get();
				if (op->status == OperationStatus::Completed)
				{
					throw ErrorCode::OperationAlreadyCompleted;
				}
				else if (op->status == OperationStatus::None)
				{
					throw ErrorCode::OperationNotStarted;
				}

				op->status = OperationStatus::Completed;
				operations.remove(op->id);

				// Notify any operations that are waiting to join this operation.
				for (const auto& blocked_id : op->blocked_operation_ids)
				{
					Operation* blocked_op = operation_map.at(blocked_id).get();
					if (blocked_op->on_join_operation(operation_id))
					{
						operations.enable(blocked_op->id);
					}
				}

				// The current operation has completed, so schedule the next enabled operation.
				schedule_next_inner(lock);
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Creates a new resource with the specified id.
		ErrorCode create_resource(size_t resource_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::create_resource] creating resource " << resource_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				auto it = resource_map.find(resource_id);
				if (it != resource_map.end())
				{
					throw ErrorCode::DuplicateResource;
				}

				resource_map.insert(std::pair<size_t, std::shared_ptr<std::unordered_set<size_t>>>(
					resource_id, std::make_shared<std::unordered_set<size_t>>()));
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Waits the resource with the specified id to become available and schedules the next operation.
		ErrorCode wait_resource(size_t resource_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}
				
				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::wait_resource] waiting resource " << resource_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				Operation* scheduled_op = operation_map.at(scheduled_op_id).get();
				scheduled_op->wait_resource_signal(resource_id);
				operations.disable(scheduled_op->id);

				auto it = resource_map.find(resource_id);
				if (it == resource_map.end())
				{
					throw ErrorCode::NotExistingResource;
				}

				std::shared_ptr<std::unordered_set<size_t>> blocked_operation_ids(it->second);
				blocked_operation_ids->insert(scheduled_op_id);

				// Waiting for the resource to be released, so schedule the next enabled operation.
				schedule_next_inner(lock);
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}
		
		// Waits the resources with the specified ids to become available and schedules the next operation.
		ErrorCode wait_resources(const size_t* resource_ids, size_t size, bool wait_all) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}

				std::unique_lock<std::mutex> lock(*mutex);
				if (wait_all)
				{
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::wait_resources] waiting all " << size << " resources" << std::endl;
	#endif // COYOTE_DEBUG_LOG
				}
				else
				{
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::wait_resources] waiting any of " << size << " resources" << std::endl;
	#endif // COYOTE_DEBUG_LOG
				}

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				Operation* scheduled_op = operation_map.at(scheduled_op_id).get();
				scheduled_op->wait_resource_signals(resource_ids, size, wait_all);
				operations.disable(scheduled_op->id);

				for (size_t i = 0; i < size; i++)
				{
					size_t resource_id = *(resource_ids + i);
					auto it = resource_map.find(resource_id);
					if (it == resource_map.end())
					{
						throw ErrorCode::NotExistingResource;
					}

					std::shared_ptr<std::unordered_set<size_t>> blocked_operation_ids(it->second);
					blocked_operation_ids->insert(scheduled_op_id);
				}

				// Waiting for the resources to be released, so schedule the next enabled operation.
				schedule_next_inner(lock);
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}
        
		// Signals all waiting operations that the resource with the specified id is available.
		ErrorCode signal_resource(size_t resource_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}

				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::signal_resource] signaling all waiting operations about resource " << resource_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				auto it = resource_map.find(resource_id);
				if (it == resource_map.end())
				{
					throw ErrorCode::NotExistingResource;
				}

				std::shared_ptr<std::unordered_set<size_t>> blocked_operation_ids(it->second);
				for (const auto& blocked_id : *blocked_operation_ids)
				{
					Operation* blocked_op = operation_map.at(blocked_id).get();
					if (blocked_op->on_resource_signal(resource_id))
					{
						operations.enable(blocked_op->id);
					}
				}

				blocked_operation_ids->clear();
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Signals the waiting operation that the resource with the specified id is available.
		ErrorCode signal_resource(size_t resource_id, size_t operation_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}

				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::signal_resource] signaling waiting operation " << operation_id << " about resource "
					<< resource_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				auto it = resource_map.find(resource_id);
				if (it == resource_map.end())
				{
					throw ErrorCode::NotExistingResource;
				}

				std::shared_ptr<std::unordered_set<size_t>> blocked_operation_ids(it->second);
				auto op_it = blocked_operation_ids->find(operation_id);
				if (op_it != blocked_operation_ids->end())
				{
					Operation* blocked_op = operation_map.at(operation_id).get();
					if (blocked_op->on_resource_signal(resource_id))
					{
						operations.enable(blocked_op->id);
					}

					blocked_operation_ids->erase(op_it);
				}
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Deletes the resource with the specified id.
		ErrorCode delete_resource(size_t resource_id) noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
			}

				std::unique_lock<std::mutex> lock(*mutex);
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::delete_resource] deleting resource " << resource_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				auto it = resource_map.find(resource_id);
				if (it == resource_map.end())
				{
					throw ErrorCode::NotExistingResource;
				}

				resource_map.erase(resource_id);
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Schedules the next operation, which can include the currently executing operation.
		// Only operations that are not blocked nor completed can be scheduled.
		ErrorCode schedule_next() noexcept
		{
			try
			{
				if (configuration->exploration_strategy() == StrategyType::None)
				{
					throw ErrorCode::SchedulerDisabled;
				}

				std::unique_lock<std::mutex> lock(*mutex);

				if (!is_attached)
				{
					throw ErrorCode::ClientNotAttached;
				}

				schedule_next_inner(lock);
			}
			catch (ErrorCode error_code)
			{
				last_error_code = error_code;
			}
			catch (...)
			{
				last_error_code = ErrorCode::Failure;
			}

			return last_error_code;
		}

		// Returns a controlled nondeterministic boolean value.
		bool next_boolean() noexcept
		{
	#ifdef COYOTE_DEBUG_LOG
			std::cout << "[coyote::next_boolean] " << std::endl;
	#endif // COYOTE_DEBUG_LOG
			return strategy->next_boolean();
		}

		// Returns a controlled nondeterministic integer value chosen from the [0, max_value) range.
		int next_integer(int max_value) noexcept
		{
	#ifdef COYOTE_DEBUG_LOG
			std::cout << "[coyote::next_integer] " << std::endl;
	#endif // COYOTE_DEBUG_LOG
			return strategy->next_integer(max_value);
		}

		// Returns the id of the currently scheduled operation.
		size_t scheduled_operation_id() noexcept
		{
			return scheduled_op_id;
		}

		// Returns a seed that can be used to reproduce the current testing iteration.
		uint64_t random_seed() noexcept
		{
			return strategy->random_seed();
		}

		// Returns the last error code, if there is one assigned.
		ErrorCode error_code() noexcept
		{
			return last_error_code;
		}

	private:
		Scheduler(Scheduler&& op) = delete;
		Scheduler(Scheduler const&) = delete;

		Scheduler& operator=(Scheduler&& op) = delete;
		Scheduler& operator=(Scheduler const&) = delete;

		std::unique_ptr<Strategy> create_strategy() noexcept
		{
			if (configuration->exploration_strategy() == StrategyType::PCT)
			{
				return std::make_unique<PCTStrategy>(configuration.get());
			}

			return std::make_unique<RandomStrategy>(configuration.get());
		}

		void create_operation_inner(size_t operation_id)
		{
			auto it = operation_map.find(operation_id);
			if (it == operation_map.end())
			{
				auto result = operation_map.insert(std::pair<size_t, std::unique_ptr<Operation>>(
					operation_id, std::make_unique<Operation>(operation_id)));
				if (operation_map.size() == 1)
				{
					// This is the first operation, so schedule it.
					scheduled_op_id = operation_id;
					result.first->second->is_scheduled = true;
				}
			}
			else
			{
				Operation* existing_op = it->second.get();
				if (existing_op->status == OperationStatus::Completed)
				{
					existing_op->status = OperationStatus::None;
					existing_op->is_scheduled = false;
				}
				else
				{
					throw ErrorCode::DuplicateOperation;
				}
			}

			auto result = operation_map.insert(std::pair<size_t, std::unique_ptr<Operation>>(
				operation_id, std::make_unique<Operation>(operation_id)));
			if (operation_map.size() == 1)
			{
				// This is the first operation, so schedule it.
				scheduled_op_id = operation_id;
				result.first->second->is_scheduled = true;
			}

			// Increment the count of created operations that have not yet started.
			pending_start_operation_count += 1;
		}

		void start_operation_inner(size_t operation_id, std::unique_lock<std::mutex>& lock)
		{
			// TODO: Check pending counter was incremented.

			auto it = operation_map.find(operation_id);
			if (it == operation_map.end())
			{
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::start_operation] not existing operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG
				throw ErrorCode::NotExistingOperation;
			}

			Operation* op = it->second.get();
			if (op->status == OperationStatus::Completed)
			{
				throw ErrorCode::OperationAlreadyCompleted;
			}
			else if (op->status != OperationStatus::None)
			{
				throw ErrorCode::OperationAlreadyStarted;
			}

			// Decrement the count of pending operations.
			pending_start_operation_count -= 1;
	#ifdef COYOTE_DEBUG_LOG
			std::cout << "[coyote::start_operation] " << pending_start_operation_count << " operations pending" << std::endl;
	#endif // COYOTE_DEBUG_LOG
			if (pending_start_operation_count == 0)
			{
				// If no pending operations remain, then release schedule next.
				pending_operations_cv.notify_all();
			}

			if (op->status != OperationStatus::Completed)
			{
				op->status = OperationStatus::Enabled;
				operations.insert(op->id);
				op->cv.notify_all();
				while (!op->is_scheduled)
				{
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::start_operation] pausing operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG
					op->cv.wait(lock);
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::start_operation] resuming operation " << operation_id << std::endl;
	#endif // COYOTE_DEBUG_LOG
					if (!is_attached)
					{
						throw ErrorCode::ClientNotAttached;
					}
				}
			}
		}

		void schedule_next_inner(std::unique_lock<std::mutex>& lock)
		{
	#ifdef COYOTE_DEBUG_LOG
			std::cout << "[coyote::schedule_next] current operation " << scheduled_op_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

			// Wait for any recently created operations to start.
			while (pending_start_operation_count > 0)
			{
	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::schedule_next] waiting " << pending_start_operation_count <<
					" pending operations" << std::endl;
	#endif // COYOTE_DEBUG_LOG
				pending_operations_cv.wait(lock);
			}

			// Check if the schedule has finished or if there is a deadlock.
			if (operations.size() == 0)
			{
				if (operations.size(false) > 0)
				{
	#ifdef COYOTE_DEBUG_LOG
					std::cout << "[coyote::schedule_next] deadlock detected" << std::endl;
	#endif // COYOTE_DEBUG_LOG
					throw ErrorCode::DeadlockDetected;
				}

	#ifdef COYOTE_DEBUG_LOG
				std::cout << "[coyote::schedule_next] no enabled operation to schedule" << std::endl;
	#endif // COYOTE_DEBUG_LOG
				throw ErrorCode::Success;
			}

			// Ask the strategy for the next operation to schedule.
			size_t next_id = strategy->next_operation(operations, scheduled_op_id);
			Operation* next_op = operation_map.at(next_id).get();

			const size_t previous_id = scheduled_op_id;
			scheduled_op_id = next_id;

	#ifdef COYOTE_DEBUG_LOG
			std::cout << "[coyote::schedule_next] next operation " << next_id << std::endl;
	#endif // COYOTE_DEBUG_LOG

			if (previous_id != next_id)
			{
				// Resume the next operation.
				next_op->is_scheduled = true;
				next_op->cv.notify_all();

				// Pause the previous operation.
				Operation* previous_op = operation_map.at(previous_id).get();
				if (previous_op->status != OperationStatus::Completed)
				{
					previous_op->is_scheduled = false;

					while (!previous_op->is_scheduled)
					{
	#ifdef COYOTE_DEBUG_LOG
						std::cout << "[coyote::schedule_next] pausing operation " << previous_id << std::endl;
	#endif // COYOTE_DEBUG_LOG
						// Wait until the operation gets scheduled again.
						previous_op->cv.wait(lock);
	#ifdef COYOTE_DEBUG_LOG
						std::cout << "[coyote::schedule_next] resuming operation " << previous_id << std::endl;
	#endif // COYOTE_DEBUG_LOG
						if (!is_attached)
						{
							throw ErrorCode::ClientNotAttached;
						}
					}
				}
			}
		}
	};
}

#endif // COYOTE_SCHEDULER_H
