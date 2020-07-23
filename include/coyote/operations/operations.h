// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_OPERATIONS_H
#define COYOTE_OPERATIONS_H

#include <vector>
#include <list>
#include <algorithm>

namespace coyote
{
	class Operations
	{
	private:
		std::vector<size_t> operation_ids;
		size_t enabled_operations_size;
		size_t disabled_operations_size;

	public:
		Operations() noexcept;

		Operations(Operations&& op) = delete;
		Operations(Operations const&) = delete;

		Operations& operator=(Operations&& op) = delete;
		Operations& operator=(Operations const&) = delete;

		size_t operator[](size_t index);

		void insert(size_t operation_id);
		void remove(size_t operation_id);
		void enable(size_t operation_id);
		void disable(size_t operation_id);

		size_t size(bool is_enabled = true);

		// Return a vector of enabled operations
		std::vector<size_t> get_enabled_operation_ids();

		void clear();

	private:
		bool find_index(size_t operation_id, size_t start, size_t end, size_t& index);
		void swap(size_t left, size_t right);
		void debug_print();
	};
}

#endif // COYOTE_OPERATIONS_H
