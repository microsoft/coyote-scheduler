// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "error_code.h"

namespace coyote
{
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
        case ErrorCode::OperationNotWaitingResource:
            return "operation is not waiting this resource";
        case ErrorCode::DuplicateResource:
            return "resource already exists";
        case ErrorCode::NotExistingResource:
            return "resource does not exist";
        case ErrorCode::ClientAttached:
            return "client is already attached to the scheduler";
        case ErrorCode::ClientNotAttached:
            return "client is not attached to the scheduler";
        default:
            return "(unknown error)";
        }
    }
}
