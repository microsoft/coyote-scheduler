// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_OPERATION_H
#define COYOTE_OPERATION_H

#include <condition_variable>
#include <unordered_set>
#include <vector>
#include "operation_status.h"

namespace coyote
{
	class Operation
	{
	private:
		// Set of operations that this operation is waiting to join.
		std::unordered_set<size_t> pending_join_operation_ids;

		// Set of resources that this operation is waiting for a signal.
		std::unordered_set<size_t> pending_signal_resource_ids;

	public:
		// The unique id of this operation.
		const size_t id;

		// The current status of this operation.
		OperationStatus status;

		// Conditional variable that can be used to block and schedule this operation.
		std::condition_variable cv;

		// Set of operations that are blocked until this operation completes.
		std::unordered_set<size_t> blocked_operation_ids;

		// True if this operation is currently scheduled, else false.
		bool is_scheduled;

		Operation(size_t operation_id) noexcept :
			id(operation_id),
			status(OperationStatus::None),
			is_scheduled(false)
		{
		}

		Operation(Operation&& op) = delete;
		Operation(Operation const&) = delete;

		Operation& operator=(Operation&& op) = delete;
		Operation& operator=(Operation const&) = delete;

		// Waits until the specified operation has completed.
		void join_operation(size_t operation_id)
		{
			status = OperationStatus::JoinAllOperations;
			pending_join_operation_ids.insert(operation_id);
		}

		// Waits until the specified operations have completed.
		void join_operations(const std::vector<size_t>& operation_ids, bool wait_all)
		{
			if (wait_all)
			{
				status = OperationStatus::JoinAllOperations;
			}
			else
			{
				status = OperationStatus::JoinAnyOperations;
			}

			for (auto& operation_id : operation_ids)
			{
				pending_join_operation_ids.insert(operation_id);
			}
		}

		// Waits until the specified resource sends a signal.
		void wait_resource_signal(size_t resource_id)
		{
			status = OperationStatus::WaitAllResources;
			pending_signal_resource_ids.insert(resource_id);
		}

		// Waits until the specified resources send a signal.
		void wait_resource_signals(const size_t* resource_ids, size_t size, bool wait_all)
		{
			if (wait_all)
			{
				status = OperationStatus::WaitAllResources;
			}
			else
			{
				status = OperationStatus::WaitAnyResource;
			}

			for (size_t i = 0; i < size; i++)
			{
				pending_signal_resource_ids.insert(*(resource_ids + i));
			}
		}

		// Invoked when the specified operation completes.
		bool on_join_operation(size_t operation_id)
		{
			pending_join_operation_ids.erase(operation_id);
			if (status == OperationStatus::JoinAllOperations && pending_join_operation_ids.empty())
			{
				// If the operation is waiting for all operations to complete, and there
				// are no more pending operations, then enable the operation.
				status = OperationStatus::Enabled;
				return true;
			}
			else if (status == OperationStatus::JoinAnyOperations)
			{
				// If the operation is waiting for at least one operation, then enable the operation,
				// and clear the set of pending operations.
				status = OperationStatus::Enabled;
				pending_join_operation_ids.clear();
				return true;
			}

			return false;
		}

		// Invoked when the specified resource sends a signal.
		bool on_resource_signal(size_t resource_id)
		{
			pending_signal_resource_ids.erase(resource_id);
			if (status == OperationStatus::WaitAllResources && pending_signal_resource_ids.empty())
			{
				// If the operation is waiting for a signal from all resources, and there
				// are no more pending resources, then enable the operation.
				status = OperationStatus::Enabled;
				return true;
			}
			else if (status == OperationStatus::WaitAnyResource)
			{
				// If the operation is waiting for at least one signal, then enable the operation,
				// and clear the set of pending resources.
				status = OperationStatus::Enabled;
				pending_signal_resource_ids.clear();
				return true;
			}

			return false;
		}
	};
}

#endif // COYOTE_OPERATION_H
