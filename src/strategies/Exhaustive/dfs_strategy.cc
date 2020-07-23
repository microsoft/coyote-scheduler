// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "strategies/Exhaustive/dfs_strategy.h"
#include <iostream>

constexpr auto FALSE_CHOICE = 0;
constexpr auto TRUE_CHOICE = 1;

namespace coyote
{

	DFSStrategy::DFSStrategy() noexcept
	{
		this->SchIndex = 0;
		this->ScheduleStack = new std::map<int, std::stack<size_t>*>();
	}

	size_t DFSStrategy::next_choice(std::vector<size_t> choices)
	{
		std::stack<size_t>* scs;

		if (this->SchIndex < this->ScheduleStack->size())
		{
			scs = this->ScheduleStack->find(this->SchIndex)->second;
		}
		else
		{
			scs = new std::stack<size_t>();
			for (std::vector<size_t>::iterator rit = choices.begin(); rit != choices.end(); ++rit)
			{
				scs->push(size_t(*rit));
			}

			this->ScheduleStack->insert(std::pair<int, std::stack<size_t>*>(this->SchIndex, scs));
		}

		size_t nextOperationChoice = scs->top();

		this->SchIndex++;
		return nextOperationChoice;
	}

	size_t DFSStrategy::next_operation(Operations& operations)
	{
		return next_choice(operations.get_enabled_operation_ids());
	}

	bool DFSStrategy::next_boolean()
	{
		std::vector<size_t> choices;
		choices.push_back(FALSE_CHOICE); // False option
		choices.push_back(TRUE_CHOICE); // True option
		size_t choice = next_choice(choices);
		if (choice == FALSE_CHOICE)
		{
			return false;
		}
		else if (choice == TRUE_CHOICE)
		{
			return true;
		}
		else
		{
			throw "Wrong choice returned by DFS.";
		}
	}

	int DFSStrategy::next_integer(int max_value)
	{
		std::vector<size_t> choices;
		for (int i = 0; i < max_value; i++)
		{
			choices.push_back((size_t)i);
		}
		
		size_t choice = next_choice(choices);
		return (int)choice;
	}

	// prepare_next_iteration() traverses the 'ScheduleStack' in reverse. 
	// For a given program point 'i', we pop the last processed operation (or) boolean (or) integer from ScheduleStack[i].
	// We break the loop if ScheduleStack[i] is non - empty after the pop, 
	// since it implies we have not explored other paths in this level ('i') fully.
	void DFSStrategy::prepare_next_iteration()
	{
		this->SchIndex = 0;

		int ScheduleStackSize = (int)this->ScheduleStack->size();
		for (int i = (ScheduleStackSize - 1); i >= 0; i--)
		{
			std::map<int, std::stack<size_t>*>::iterator it = this->ScheduleStack->find(i);
			std::stack<size_t>* s_scs = it->second;
			s_scs->pop();
			if (s_scs->size() == 0)
			{
				this->ScheduleStack->erase(it);
			}
			else
			{
				return;
			}
		}
	}

	bool DFSStrategy::is_fair()
	{
		return false;
	}

	std::string DFSStrategy::get_description()
	{
		return "DFS Strategy.";
	}
}
