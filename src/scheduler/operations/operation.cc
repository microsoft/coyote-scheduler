// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <iostream>
#include "operation.h"

namespace coyote
{
	Operation::Operation(size_t operation_id) noexcept :
		id(operation_id),
		status(OperationStatus::None),
		is_scheduled(false)
	{
	}

	void Operation::join_operation(size_t operation_id)
	{
		status = OperationStatus::JoinAllOperations;
		pending_join_operation_ids.insert(operation_id);
	}

	void Operation::join_operations(const size_t* operation_ids, int size, bool wait_all)
	{
		if (wait_all)
		{
			status = OperationStatus::JoinAllOperations;
		}
		else
		{
			status = OperationStatus::JoinAnyOperations;
		}

		for (int i = 0; i < size; i++)
		{
			pending_join_operation_ids.insert(*(operation_ids + i));
		}
	}

	void Operation::wait_resource_signal(size_t resource_id)
	{
		status = OperationStatus::WaitAllResources;
		pending_signal_resource_ids.insert(resource_id);
	}

	void Operation::wait_resource_signals(const size_t* resource_ids, int size, bool wait_all)
	{
		if (wait_all)
		{
			status = OperationStatus::WaitAllResources;
		}
		else
		{
			status = OperationStatus::WaitAnyResource;
		}

		for (int i = 0; i < size; i++)
		{
			pending_signal_resource_ids.insert(*(resource_ids + i));
		}
	}

	void Operation::on_join_operation(size_t operation_id)
	{
		pending_join_operation_ids.erase(operation_id);
		if (status == OperationStatus::JoinAllOperations && pending_join_operation_ids.empty())
		{
			// If the operation is waiting for all operations to complete, and there
			// are no more pending operations, then enable the operation.
			status = OperationStatus::Enabled;
		}
		else if (status == OperationStatus::JoinAnyOperations)
		{
			// If the operation is waiting for at least one operation, then enable the operation,
			// and clear the set of pending operations.
			status = OperationStatus::Enabled;
			pending_join_operation_ids.clear();
		}
	}

	void Operation::on_resource_signal(size_t resource_id)
	{
		pending_signal_resource_ids.erase(resource_id);
		if (status == OperationStatus::WaitAllResources && pending_signal_resource_ids.empty())
		{
			// If the operation is waiting for a signal from all resources, and there
			// are no more pending resources, then enable the operation.
			status = OperationStatus::Enabled;
		}
		else if (status == OperationStatus::WaitAnyResource)
		{
			// If the operation is waiting for at least one signal, then enable the operation,
			// and clear the set of pending resources.
			status = OperationStatus::Enabled;
			pending_signal_resource_ids.clear();
		}
	}
}
