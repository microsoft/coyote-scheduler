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
}

#endif // COYOTE_ERROR_CODE_H
