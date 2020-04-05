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
        Failure = 1,
        ScheduleExplored = 100,
        DuplicateOperation = 200,
        NotExistingOperation = 201,
        MainOperationExplicitlyCreated = 202,
        MainOperationExplicitlyStarted = 203,
        MainOperationExplicitlyCompleted = 204,
        OperationAlreadyStarted = 205,
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
