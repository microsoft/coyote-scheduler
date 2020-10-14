// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <iostream>
#include <vector>
#include "scheduler.h"
#include "operations/operation_status.h"

namespace coyote
{
	Scheduler::Scheduler() noexcept :
		Scheduler(std::make_unique<Settings>())
	{
	}

	Scheduler::Scheduler(std::unique_ptr<Settings> settings) noexcept :
		configuration(std::move(settings)),
		strategy(std::make_unique<RandomStrategy>(configuration.get())),
		mutex(std::make_unique<std::mutex>()),
		pending_operations_cv(),
		scheduled_operation_id(0),
		pending_start_operation_count(0),
		is_attached(false),
		iteration_count(0),
		last_error_code(ErrorCode::Success)
	{
	}

	ErrorCode Scheduler::attach() noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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
				strategy->prepare_next_iteration();
			}

			create_operation_inner(main_operation_id);
			start_operation_inner(main_operation_id, lock);
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

	ErrorCode Scheduler::detach() noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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

			Operation* main_op = operation_map.at(main_operation_id).get();
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

	ErrorCode Scheduler::create_operation(size_t operation_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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
			else if (operation_id == main_operation_id)
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

	ErrorCode Scheduler::start_operation(size_t operation_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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
			else if (operation_id == main_operation_id)
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

	ErrorCode Scheduler::join_operation(size_t operation_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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
				join_op->blocked_operation_ids.insert(scheduled_operation_id);

				Operation* scheduled_op = operation_map.at(scheduled_operation_id).get();
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

	ErrorCode Scheduler::join_operations(const size_t* operation_ids, size_t size, bool wait_all) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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
					join_op->blocked_operation_ids.insert(scheduled_operation_id);
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
				Operation* scheduled_op = operation_map.at(scheduled_operation_id).get();
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

	ErrorCode Scheduler::complete_operation(size_t operation_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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
			else if (operation_id == main_operation_id)
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
			operations.disable(op->id);

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

	ErrorCode Scheduler::create_resource(size_t resource_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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

	ErrorCode Scheduler::wait_resource(size_t resource_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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

			Operation* scheduled_op = operation_map.at(scheduled_operation_id).get();
			scheduled_op->wait_resource_signal(resource_id);
			operations.disable(scheduled_op->id);

			auto it = resource_map.find(resource_id);
			if (it == resource_map.end())
			{
				throw ErrorCode::NotExistingResource;
			}

			std::shared_ptr<std::unordered_set<size_t>> blocked_operation_ids(it->second);
			blocked_operation_ids->insert(scheduled_operation_id);

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

	ErrorCode Scheduler::wait_resources(const size_t* resource_ids, size_t size, bool wait_all) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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

			Operation* scheduled_op = operation_map.at(scheduled_operation_id).get();
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
				blocked_operation_ids->insert(scheduled_operation_id);
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

	ErrorCode Scheduler::signal_resource(size_t resource_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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

	ErrorCode Scheduler::signal_resource(size_t resource_id, size_t operation_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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

	ErrorCode Scheduler::delete_resource(size_t resource_id) noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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

	ErrorCode Scheduler::schedule_next() noexcept
	{
		try
		{
			if (configuration->exploration_strategy() == Strategy::None)
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

	bool Scheduler::next_boolean() noexcept
	{
#ifdef COYOTE_DEBUG_LOG
		std::cout << "[coyote::next_boolean] " << std::endl;
#endif // COYOTE_DEBUG_LOG
		return strategy->next_boolean();
	}

	size_t Scheduler::next_integer(size_t max_value) noexcept
	{
#ifdef COYOTE_DEBUG_LOG
		std::cout << "[coyote::next_integer] " << std::endl;
#endif // COYOTE_DEBUG_LOG
		return strategy->next_integer(max_value);
	}

	size_t Scheduler::random_seed() noexcept
	{
		return strategy->random_seed();
	}

	ErrorCode Scheduler::error_code() noexcept
	{
		return last_error_code;
	}

	void Scheduler::create_operation_inner(size_t operation_id)
	{
		auto it = operation_map.find(operation_id);
		if (it != operation_map.end())
		{
			throw ErrorCode::DuplicateOperation;
		}

		auto result = operation_map.insert(std::pair<size_t, std::unique_ptr<Operation>>(
			operation_id, std::make_unique<Operation>(operation_id)));
		if (operation_map.size() == 1)
		{
			// This is the first operation, so schedule it.
			scheduled_operation_id = operation_id;
			result.first->second->is_scheduled = true;
		}

		// Increment the count of created operations that have not yet started.
		pending_start_operation_count += 1;
	}

	void Scheduler::start_operation_inner(size_t operation_id, std::unique_lock<std::mutex>& lock)
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

	void Scheduler::schedule_next_inner(std::unique_lock<std::mutex>& lock)
	{
#ifdef COYOTE_DEBUG_LOG
		std::cout << "[coyote::schedule_next] current operation " << scheduled_operation_id << std::endl;
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
		size_t next_id = strategy->next_operation(operations);
		Operation* next_op = operation_map.at(next_id).get();

		const size_t previous_id = scheduled_operation_id;
		scheduled_operation_id = next_id;

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
}
