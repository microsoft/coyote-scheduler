// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef TEST_H
#define TEST_H

#include <iostream>
#include "scheduler.h"

void assert(bool predicate, std::string error)
{
	if (!predicate)
	{
		throw error;
	}
}

void assert(std::error_code actual, coyote::ErrorCode expected)
{
	std::error_code expected_error_code = expected;
	if (actual.value() != expected_error_code.value())
	{
		throw "expected the '" + expected_error_code.message() + "' error code, but received '" +
			actual.message() + "'.";
	}
}

#endif // TEST_H
