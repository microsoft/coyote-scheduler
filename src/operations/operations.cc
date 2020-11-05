// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_DEBUG_LOG_V2
//#define COYOTE_DEBUG_LOG_V2
#endif // COYOTE_DEBUG_LOG_V2

#include <iostream>
#include <iterator>
#include <vector>
#include "operations/operations.h"

namespace coyote
{
	Operations::Operations() noexcept :
		enabled_operations_size(0),
		disabled_operations_size(0)
	{
	}

	size_t Operations::operator[](size_t index)
	{
		return operation_ids[index];
	}

	void Operations::insert(size_t operation_id)
	{
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "insert: " << operation_id << std::endl;
		std::cout << "pre-insert-total/enabled/disabled: " << operation_ids.size() << "/" << enabled_operations_size << "/" << disabled_operations_size << std::endl;
		debug_print();
#endif // COYOTE_DEBUG_LOG_V2
		operation_ids.push_back(operation_id);
		enabled_operations_size += 1;
		if (operation_ids.size() != enabled_operations_size)
		{
			swap(operation_ids.size() - 1, enabled_operations_size - 1);
		}
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "post-insert-total/enabled/disabled: " << operation_ids.size() << "/" << enabled_operations_size << "/" << disabled_operations_size << std::endl;
		debug_print();
#endif // COYOTE_DEBUG_LOG_V2
	}

	void Operations::remove(size_t operation_id)
	{
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "remove: " << operation_id << std::endl;
		std::cout << "pre-remove-total/enabled/disabled: " << operation_ids.size() << "/" << enabled_operations_size << "/" << disabled_operations_size << std::endl;
		debug_print();
#endif // COYOTE_DEBUG_LOG_V2
		bool found = false;
		size_t index;
		if (find_index(operation_id, 0, enabled_operations_size, index))
		{
			enabled_operations_size -= 1;
			found = true;
		}
		else if (find_index(operation_id, enabled_operations_size, operation_ids.size(), index))
		{
			disabled_operations_size -= 1;
			found = true;
		}

		if (found)
		{
#ifdef COYOTE_DEBUG_LOG_V2
			std::cout << "remove-swap: " << index << "-" << enabled_operations_size << "-" << disabled_operations_size << std::endl;
#endif // COYOTE_DEBUG_LOG_V2
			swap(index, enabled_operations_size);
			swap(enabled_operations_size, operation_ids.size() - 1);
			operation_ids.pop_back();
		}
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "post-remove-total/enabled/disabled: " << operation_ids.size() << "/" << enabled_operations_size << "/" << disabled_operations_size << std::endl;
		debug_print();
#endif // COYOTE_DEBUG_LOG_V2
	}

	void Operations::enable(size_t operation_id)
	{
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "enable: " << operation_id << std::endl;
		std::cout << "pre-enable-total/enabled/disabled: " << operation_ids.size() << "/" << enabled_operations_size << "/" << disabled_operations_size << std::endl;
		debug_print();
#endif // COYOTE_DEBUG_LOG_V2
		size_t index;
		if (find_index(operation_id, enabled_operations_size, operation_ids.size(), index))
		{
#ifdef COYOTE_DEBUG_LOG_V2
			std::cout << "enable-swap: " << index << "-" << enabled_operations_size << "-" << disabled_operations_size << std::endl;
#endif // COYOTE_DEBUG_LOG_V2
			swap(index, enabled_operations_size);
			enabled_operations_size += 1;
			disabled_operations_size -= 1;
		}
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "post-enable-total/enabled/disabled: " << operation_ids.size() << "/" << enabled_operations_size << "/" << disabled_operations_size << std::endl;
		debug_print();
#endif // COYOTE_DEBUG_LOG_V2
	}

	void Operations::disable(size_t operation_id)
	{
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "disable: " << operation_id << std::endl;
		std::cout << "pre-disable-total/enabled/disabled: " << operation_ids.size() << "/" << enabled_operations_size << "/" << disabled_operations_size << std::endl;
		debug_print();
#endif // COYOTE_DEBUG_LOG_V2
		size_t index;
		if (find_index(operation_id, 0, enabled_operations_size, index))
		{
			enabled_operations_size -= 1;
			disabled_operations_size += 1;
#ifdef COYOTE_DEBUG_LOG_V2
			std::cout << "disable-swap: " << index << "-" << enabled_operations_size << "-" << disabled_operations_size << std::endl;
#endif // COYOTE_DEBUG_LOG_V2
			swap(index, enabled_operations_size);
		}
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "post-disable-total/enabled/disabled: " << operation_ids.size() << "/" << enabled_operations_size << "/" << disabled_operations_size << std::endl;
		debug_print();
#endif // COYOTE_DEBUG_LOG_V2
	}

	size_t Operations::size(bool is_enabled)
	{
		return is_enabled ? enabled_operations_size : disabled_operations_size;
	}

	void Operations::clear()
	{
		operation_ids.clear();
		enabled_operations_size = 0;
		disabled_operations_size = 0;
	}

	bool Operations::find_index(size_t operation_id, size_t start, size_t end, size_t& index)
	{
		for (index = start; index < end; index++)
		{
			if (operation_ids[index] == operation_id)
			{
				return true;
			}
		}

		return false;
	}

	void Operations::swap(size_t left, size_t right)
	{
		if (left != right)
		{
			size_t temp = operation_ids[left];
			operation_ids[left] = operation_ids[right];
			operation_ids[right] = temp;
		}
	}

	void Operations::debug_print()
	{
#ifdef COYOTE_DEBUG_LOG_V2
		std::cout << "enabled: ";
		for (size_t index = 0; index < enabled_operations_size; index++)
		{
			if (index == 0)
			{
				std::cout << operation_ids[index];
			}
			else
			{
				std::cout << ", " << operation_ids[index];
			}
		}

		std::cout << std::endl << "disabled: ";
		for (size_t index = enabled_operations_size; index < operation_ids.size(); index++)
		{
			if (index == enabled_operations_size)
			{
				std::cout << operation_ids[index];
			}
			else
			{
				std::cout << ", " << operation_ids[index];
			}
		}

		std::cout << std::endl;
#endif // COYOTE_DEBUG_LOG_V2
	}
}
