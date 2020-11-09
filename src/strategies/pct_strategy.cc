// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "strategies/pct_strategy.h"
#include <iostream>

namespace coyote
{
	PCTStrategy::PCTStrategy(size_t seed, int max_priority_switch_points) noexcept :
		iteration_seed(seed),
		generator(seed),
		max_priority_switch_points(max_priority_switch_points)
	{
		this->schedule_length = 0;
		this->scheduled_steps = 0;
		this->prioritized_operations = new std::list<size_t>();
		this->priority_change_points = new std::set<int>();
	}
 
	int PCTStrategy::next_operation(Operations& operations)
	{
		std::vector<size_t> enabled_oprs = operations.get_enabled_operation_ids();
		this->scheduled_steps++;
		return get_prioritized_operation(enabled_oprs);
	}

	bool PCTStrategy::next_boolean()
	{
		this->scheduled_steps++;
		return generator.next() & 1;
	}

	int PCTStrategy::next_integer(int max_value)
	{
		this->scheduled_steps++;
		return generator.next() % max_value;
	}

	uint64_t PCTStrategy::random_seed()
	{
		return iteration_seed;
	}

	void PCTStrategy::prepare_next_iteration()
	{
		if (this->schedule_length < this->scheduled_steps)
		{
			this->schedule_length = this->scheduled_steps;
		}

		this->scheduled_steps = 0;
		this->prioritized_operations = new std::list<size_t>();
		this->priority_change_points = new std::set<int>();

		std::list<int> range;
		for (int i = 0; i < this->schedule_length; i++)
		{
			range.push_back(i);
		}

		// Shuffling the "range" list using Fisher-Yates algorithm.
		for (int idx = (range.size() - 1); idx >= 1; idx--)
		{
			int point = this->generator.next() % this->schedule_length;
			
			std::list<int>::iterator idx_itr = range.begin();
			std::advance(idx_itr, idx);

			std::list<int>::iterator point_itr = range.begin();
			std::advance(point_itr, point);

			std::iter_swap(idx_itr, point_itr);
		}

		std::list<int>::iterator pri_it = range.begin();
		for (int i = 0; i < this->max_priority_switch_points && pri_it != range.end(); i++)
		{
			this->priority_change_points->insert(*pri_it);
			pri_it++;
		}
	}

	size_t PCTStrategy::get_prioritized_operation(std::vector<size_t> ops)
	{
		for (std::vector<size_t>::iterator it = ops.begin(); it != ops.end(); it++)
		{
			bool flag = true;
			for (std::list<size_t>::iterator pri_it = this->prioritized_operations->begin(); pri_it != this->prioritized_operations->end(); pri_it++)
			{
				if (*it == *pri_it)
				{
					flag = false;
					break;
				}
			}

			if (flag)
			{
				auto mIndex = this->generator.next() % (this->prioritized_operations->size() + 1);
				std::list<size_t>::iterator iter = this->prioritized_operations->begin();
				std::advance(iter, mIndex);
				this->prioritized_operations->insert(iter, *it);
			}
		}

		if (this->priority_change_points->find(this->scheduled_steps) != this->priority_change_points->end())
		{
			if (ops.size() == 1)
			{
				move_priority_change_point_forward();
			}
			else
			{
				size_t opr = get_highest_priority_enabled_operation(ops);
				this->prioritized_operations->remove(opr);
				this->prioritized_operations->push_back(opr);
			}
		}

		return get_highest_priority_enabled_operation(ops);
	}

	size_t PCTStrategy::get_highest_priority_enabled_operation(std::vector<size_t> choices)
	{
		for (std::list<size_t>::iterator pri_it = this->prioritized_operations->begin(); pri_it != this->prioritized_operations->end(); pri_it++)
		{
			for (std::vector<size_t>::iterator it = choices.begin(); it != choices.end(); it++)
			{
				if (*it == *pri_it)
				{
					return *it;
				}
			}
		}
	}

	void PCTStrategy::move_priority_change_point_forward()
	{
		this->priority_change_points->erase(this->scheduled_steps);
		int new_priority_change_point = this->scheduled_steps + 1;
		while (this->priority_change_points->find(new_priority_change_point) != this->priority_change_points->end())
		{
			new_priority_change_point = new_priority_change_point + 1;
		}

		this->priority_change_points->insert(new_priority_change_point);
	}
}
