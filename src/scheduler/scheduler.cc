// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <chrono>
#include <iostream>
#include <vector>
#include "scheduler.h"

namespace coyote
{
	Scheduler::Scheduler() noexcept :
		Scheduler(std::chrono::high_resolution_clock::now().time_since_epoch().count())
	{
	}

	Scheduler::Scheduler(size_t seed) noexcept :
		strategy(std::make_unique<RandomStrategy>(seed)),
		mutex(std::make_unique<std::mutex>()),
		pending_operations_cv(),
		scheduled_operation_id(0),
		pending_start_operation_count(0),
		is_attached(false),
		last_error_code(ErrorCode::Success)
	{
	}

	std::error_code Scheduler::attach() noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::attach] attaching the current operation" << std::endl;
#endif // NDEBUG

			if (is_attached)
			{
				throw ErrorCode::ClientAttached;
			}

			is_attached = true;
			last_error_code = ErrorCode::Success;

			create_operation(main_operation_id, lock);
			start_operation(main_operation_id, lock);
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

	std::error_code Scheduler::detach() noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::detach] releasing all operations" << std::endl;
#endif // NDEBUG

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}

			is_attached = false;

			std::shared_ptr<Operation> op(operation_map.at(main_operation_id));
			op->status = OperationStatus::Completed;

			for (auto& kvp : operation_map)
			{
				Operation* op = kvp.second.get();
				if (op->status != OperationStatus::Completed)
				{
#ifndef NDEBUG
					std::cout << "[coyote::detach] canceling operation " << op->id << std::endl;
#endif // NDEBUG
					// If the operation has not already completed, then cancel it.
					op->is_scheduled = true;
					op->status = OperationStatus::Canceled;
					op->cv.notify_all();
				}
			}

			operation_map.clear();
			resource_map.clear();
			pending_start_operation_count = 0;
			std::cout << "[coyote::detach] done" << std::endl;
			strategy->prepare_next_iteration();
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

	std::error_code Scheduler::create_operation(size_t operation_id) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::create_operation] creating operation " << operation_id << std::endl;
#endif // NDEBUG

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}
			else if (operation_id == main_operation_id)
			{
				throw ErrorCode::MainOperationExplicitlyCreated;
			}

			create_operation(operation_id, lock);
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

	void Scheduler::create_operation(size_t operation_id, std::unique_lock<std::mutex>& lock)
	{
		auto it = operation_map.find(operation_id);
		if (it != operation_map.end())
		{
			throw ErrorCode::DuplicateOperation;
		}

		auto result = operation_map.insert(std::pair<size_t, std::shared_ptr<Operation>>(
			operation_id, std::make_shared<Operation>(operation_id)));
		if (operation_map.size() == 1)
		{
			// This is the first operation, so schedule it.
			scheduled_operation_id = operation_id;
			result.first->second->is_scheduled = true;
		}

		// Increment the count of created operations that have not yet started.
		pending_start_operation_count += 1;
	}

	std::error_code Scheduler::start_operation(size_t operation_id) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::start_operation] starting operation " << operation_id << std::endl;
#endif // NDEBUG

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}
			else if (operation_id == main_operation_id)
			{
				throw ErrorCode::MainOperationExplicitlyStarted;
			}

			start_operation(operation_id, lock);
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

	void Scheduler::start_operation(size_t operation_id, std::unique_lock<std::mutex>& lock)
	{
		// TODO: Check pending counter was incremented.

		auto it = operation_map.find(operation_id);
		if (it == operation_map.end())
		{
#ifndef NDEBUG
			std::cout << "[coyote::start_operation] not existing operation " << operation_id << std::endl;
#endif // NDEBUG
			throw ErrorCode::NotExistingOperation;
		}

		std::shared_ptr<Operation> op(it->second);
		if (op->status != OperationStatus::None)
		{
			throw ErrorCode::OperationAlreadyStarted;
		}

		// Decrement the count of pending operations.
		pending_start_operation_count -= 1;
#ifndef NDEBUG
		std::cout << "[coyote::start_operation] " << pending_start_operation_count << " operations pending" << std::endl;
#endif // NDEBUG
		if (pending_start_operation_count == 0)
		{
#ifndef NDEBUG
			std::cout << "[coyote::start_operation] no pending operations" << std::endl;
#endif // NDEBUG

			// If no pending operations remain, then release schedule next.
			pending_operations_cv.notify_all();
		}

		if (op->status != OperationStatus::Canceled)
		{
			op->status = OperationStatus::Enabled;
			op->cv.notify_all();
			while (!op->is_scheduled)
			{
#ifndef NDEBUG
				std::cout << "[coyote::start_operation] pausing operation " << operation_id << std::endl;
#endif // NDEBUG
				op->cv.wait(lock);
#ifndef NDEBUG
				std::cout << "[coyote::start_operation] resuming operation " << operation_id << std::endl;
#endif // NDEBUG
			}
		}
	}

	std::error_code Scheduler::join_operation(size_t operation_id) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::join_operation] joining operation " << operation_id << std::endl;
#endif // NDEBUG

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}

			std::shared_ptr<Operation> op(operation_map.at(scheduled_operation_id));
			op->join_operation(operation_id);

			auto it = operation_map.find(operation_id);
			if (it == operation_map.end())
			{
#ifndef NDEBUG
				std::cout << "[coyote::join_operation] not existing operation " << operation_id << std::endl;
#endif // NDEBUG
				throw ErrorCode::NotExistingOperation;
			}

			it->second->blocked_operation_ids.insert(scheduled_operation_id);

			// Waiting for the resource to be released, so schedule the next enabled operation.
			schedule_next(lock);
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

	std::error_code Scheduler::join_operations(const size_t* operation_ids, int size, bool wait_all) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
			if (wait_all)
			{
#ifndef NDEBUG
				std::cout << "[coyote::join_operations] joining all " << size << " operations" << std::endl;
#endif // NDEBUG
			}
			else
			{
#ifndef NDEBUG
				std::cout << "[coyote::join_operations] joining any of " << size << " operations" << std::endl;
#endif // NDEBUG
			}

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}

			std::shared_ptr<Operation> op(operation_map.at(scheduled_operation_id));
			op->join_operations(operation_ids, size, wait_all);

			for (int i = 0; i < size; i++)
			{
				size_t operation_id = *(operation_ids + i);
				auto it = operation_map.find(operation_id);
				if (it == operation_map.end())
				{
#ifndef NDEBUG
					std::cout << "[coyote::join_operations] not existing operation " << operation_id << std::endl;
#endif // NDEBUG
					throw ErrorCode::NotExistingOperation;
				}

				it->second->blocked_operation_ids.insert(scheduled_operation_id);
			}

			// Waiting for the resources to be released, so schedule the next enabled operation.
			schedule_next(lock);
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

	std::error_code Scheduler::complete_operation(size_t operation_id) noexcept
	{
		// TODO: disallow completing main operation.

		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::complete_operation] completing operation " << operation_id << std::endl;
#endif // NDEBUG

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}
			else if (operation_id == main_operation_id)
			{
				throw ErrorCode::MainOperationExplicitlyCompleted;
			}

			std::shared_ptr<Operation> op(operation_map.at(operation_id));
			op->status = OperationStatus::Completed;

			// Notify any operations that are waiting to join this operation.
			for (const auto& blocked_id : op->blocked_operation_ids)
			{
				std::shared_ptr<Operation> op(operation_map.at(blocked_id));
				op->on_join_operation(operation_id);
			}

			// The current operation has completed, so schedule the next enabled operation.
			schedule_next(lock);
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

	std::error_code Scheduler::create_resource(size_t resource_id) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::create_resource] creating resource " << resource_id << std::endl;
#endif // NDEBUG

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

	std::error_code Scheduler::wait_resource(size_t resource_id) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::wait_resource] waiting resource " << resource_id << std::endl;
#endif // NDEBUG

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}

			std::shared_ptr<Operation> op(operation_map.at(scheduled_operation_id));
			op->wait_resource_signal(resource_id);

			auto it = resource_map.find(resource_id);
			if (it == resource_map.end())
			{
				throw ErrorCode::NotExistingResource;
			}

			std::shared_ptr<std::unordered_set<size_t>> blocked_operation_ids(it->second);
			blocked_operation_ids->insert(scheduled_operation_id);

			// Waiting for the resource to be released, so schedule the next enabled operation.
			schedule_next(lock);
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

	std::error_code Scheduler::wait_resources(const size_t* resource_ids, int size, bool wait_all) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
			if (wait_all)
			{
#ifndef NDEBUG
				std::cout << "[coyote::wait_resources] waiting all " << size << " resources" << std::endl;
#endif // NDEBUG
			}
			else
			{
#ifndef NDEBUG
				std::cout << "[coyote::wait_resources] waiting any of " << size << " resources" << std::endl;
#endif // NDEBUG
			}

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}

			std::shared_ptr<Operation> op(operation_map.at(scheduled_operation_id));
			op->wait_resource_signals(resource_ids, size, wait_all);

			for (int i = 0; i < size; i++)
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
			schedule_next(lock);
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

	std::error_code Scheduler::signal_resource(size_t resource_id) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::signal_resource] signaling resource " << resource_id << std::endl;
#endif // NDEBUG

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
				std::shared_ptr<Operation> op(operation_map.at(blocked_id));
				op->on_resource_signal(resource_id);
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

	std::error_code Scheduler::delete_resource(size_t resource_id) noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);
#ifndef NDEBUG
			std::cout << "[coyote::delete_resource] deleting resource " << resource_id << std::endl;
#endif // NDEBUG

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

	std::error_code Scheduler::schedule_next() noexcept
	{
		try
		{
			std::unique_lock<std::mutex> lock(*mutex);

			if (!is_attached)
			{
				throw ErrorCode::ClientNotAttached;
			}

			schedule_next(lock);
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

	void Scheduler::schedule_next(std::unique_lock<std::mutex>& lock)
	{
#ifndef NDEBUG
		std::cout << "[coyote::schedule_next] current operation " << scheduled_operation_id << std::endl;
#endif // NDEBUG

		// Wait for any recently created operations to start.
		while (pending_start_operation_count > 0)
		{
#ifndef NDEBUG
			std::cout << "[coyote::schedule_next] waiting " << pending_start_operation_count <<
				" pending operations" << std::endl;
#endif // NDEBUG
			pending_operations_cv.wait(lock);
#ifndef NDEBUG
			std::cout << "[coyote::schedule_next] waited " << pending_start_operation_count <<
				" pending operations" << std::endl;
#endif // NDEBUG
		}

		const size_t previous_id = scheduled_operation_id;
		std::shared_ptr<Operation> previous_op(operation_map.at(previous_id));

		// Ask the strategy for the next operation to schedule.
		const std::optional<size_t> next_id = strategy->next_operation(operation_map);
		if (!next_id)
		{
#ifndef NDEBUG
			std::cout << "[coyote::schedule_next] no enabled operation to schedule" << std::endl;
#endif // NDEBUG
			throw ErrorCode::ScheduleExplored;
		}

		scheduled_operation_id = *next_id;
		std::shared_ptr<Operation> next_op(operation_map.at(scheduled_operation_id));

#ifndef NDEBUG
		std::cout << "[coyote::schedule_next] next operation " << scheduled_operation_id << std::endl;
#endif // NDEBUG

		if (previous_id != next_id)
		{
			// Resume the next operation.
			next_op->is_scheduled = true;
			next_op->cv.notify_all();

			// Pause the previous operation.
			if (previous_op->status != OperationStatus::Completed)
			{
				//std::unique_lock<std::mutex> lock(*mutex);
				previous_op->is_scheduled = false;

				while (!previous_op->is_scheduled)
				{
#ifndef NDEBUG
					std::cout << "[coyote::schedule_next] pausing operation " << previous_id << std::endl;
#endif // NDEBUG
					// Wait until the operation gets scheduled again.
					previous_op->cv.wait(lock);
#ifndef NDEBUG
					std::cout << "[coyote::schedule_next] resuming operation " << previous_id << std::endl;
#endif // NDEBUG
				}
			}
		}
	}

	bool Scheduler::next_boolean() noexcept
	{
#ifndef NDEBUG
		std::cout << "[coyote::next_boolean] " << std::endl;
#endif // NDEBUG
		return strategy->next_boolean();
	}

	int Scheduler::next_integer(int max_value) noexcept
	{
#ifndef NDEBUG
		std::cout << "[coyote::next_integer] " << std::endl;
#endif // NDEBUG
		return strategy->next_integer(max_value);
	}

	size_t Scheduler::seed() noexcept
	{
		return strategy->seed();
	}

	std::error_code Scheduler::error_code() noexcept
	{
		return last_error_code;
	}
}
