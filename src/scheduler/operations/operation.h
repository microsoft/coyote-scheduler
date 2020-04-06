// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef OPERATION_H
#define OPERATION_H

#include <condition_variable>
#include <cstdint>
#include <memory>
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

		Operation(size_t operation_id) noexcept;
		Operation(Operation&& op) = delete;
		Operation(Operation const&) = delete;

		Operation& operator=(Operation&& op) = delete;
		Operation& operator=(Operation const&) = delete;

		// Waits until the specified operation has completed.
		void join_operation(size_t operation_id);

		// Waits until the specified operations have completed.
		void join_operations(const std::vector<size_t>& operation_ids, bool wait_all);

		// Waits until the specified resource sends a signal.
		void wait_resource_signal(size_t resource_id);

		// Waits until the specified resources send a signal.
		void wait_resource_signals(const size_t* resource_ids, int size, bool wait_all);

		// Invoked when the specified operation completes.
		void on_join_operation(size_t operation_id);

		// Invoked when the specified resource sends a signal.
		void on_resource_signal(size_t resource_id);
	};
}

#endif // OPERATION_H
