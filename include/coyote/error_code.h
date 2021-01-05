// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COYOTE_ERROR_CODE_H
#define COYOTE_ERROR_CODE_H

#include <string>

namespace coyote
{
    enum class ErrorCode
    {
        Success = 0,
        Failure = 100,
        DeadlockDetected = 101,
        DuplicateOperation = 200,
        NotExistingOperation = 201,
        MainOperationExplicitlyCreated = 202,
        MainOperationExplicitlyStarted = 203,
        MainOperationExplicitlyCompleted = 204,
        OperationNotStarted = 205,
        OperationAlreadyStarted = 206,
        OperationAlreadyCompleted = 207,
        DuplicateResource = 300,
        NotExistingResource = 301,
        ClientAttached = 400,
        ClientNotAttached = 401,
        InternalError = 500,
        SchedulerDisabled = 501
    };

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
}

#endif // COYOTE_ERROR_CODE_H
