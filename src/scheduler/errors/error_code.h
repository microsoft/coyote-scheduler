// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#include <system_error>

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
        ClientNotAttached = 401
    };

    std::error_code make_error_code(ErrorCode error_code);
}

namespace std
{
    template <>
    struct is_error_code_enum<coyote::ErrorCode> : true_type {};
}

#endif // ERROR_CODE_H
