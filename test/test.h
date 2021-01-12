// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_TEST_H
#define COYOTE_TEST_H

#include <chrono>
#include <iostream>
#include "coyote/scheduler.h"

using namespace coyote;

std::string error_message(ErrorCode error_code)
{
		switch (error_code)
		{
		case ErrorCode::Success:
				return "success";
		case ErrorCode::Failure:
				return "failure";
		case ErrorCode::DeadlockDetected:
				return "deadlock detected";
		case ErrorCode::DuplicateOperation:
				return "operation already exists";
		case ErrorCode::NotExistingOperation:
				return "operation does not exist";
		case ErrorCode::MainOperationExplicitlyCreated:
				return "not allowed to explicitly create main operation '0'";
		case ErrorCode::MainOperationExplicitlyStarted:
				return "not allowed to explicitly start main operation '0'";
		case ErrorCode::MainOperationExplicitlyCompleted:
				return "not allowed to explicitly complete main operation '0'";
		case ErrorCode::OperationNotStarted:
				return "operation has not started";
		case ErrorCode::OperationAlreadyStarted:
				return "operation has already started";
		case ErrorCode::OperationAlreadyCompleted:
				return "operation has already completed";
		case ErrorCode::DuplicateResource:
				return "resource already exists";
		case ErrorCode::NotExistingResource:
				return "resource does not exist";
		case ErrorCode::ClientAttached:
				return "client is already attached to the scheduler";
		case ErrorCode::ClientNotAttached:
				return "client is not attached to the scheduler";
		case ErrorCode::InternalError:
				return "internal error";
		case ErrorCode::SchedulerDisabled:
				return "scheduler is disabled";
		default:
				return "(unknown error)";
		}
}

void assert(bool predicate, std::string error)
{
	if (!predicate)
	{
		throw error;
	}
}

void assert(ErrorCode actual, ErrorCode expected)
{
	if (actual != expected)
	{
		throw "expected the '" + error_message(expected) + "' error code, but received '" + error_message(actual) + "'.";
	}
}

size_t total_time(std::chrono::steady_clock::time_point start_time)
{
	auto end_time = std::chrono::steady_clock::now();
	return (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
}

#endif // COYOTE_TEST_H
