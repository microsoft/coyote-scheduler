// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "error_code.h"

namespace coyote
{
    struct ErrorCategory : std::error_category
    {
        const char* name() const noexcept override;
        std::string message(int ev) const override;
    };

    const char* ErrorCategory::name() const noexcept
    {
        return "coyote";
    }

    std::string ErrorCategory::message(int ev) const
    {
        switch (static_cast<ErrorCode>(ev))
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
        default:
            return "(unknown error)";
        }
    }

    const ErrorCategory error_category{};
}

std::error_code coyote::make_error_code(ErrorCode error_code)
{
    return { static_cast<int>(error_code), coyote::error_category };
}
