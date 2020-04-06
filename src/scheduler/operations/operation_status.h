// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef OPERATION_STATUS_H
#define OPERATION_STATUS_H

namespace coyote
{
    enum class OperationStatus
    {
        None = 0,
        Enabled,
        JoinAnyOperations,
        JoinAllOperations,
        WaitAnyResource,
        WaitAllResources,
        Completed
    };
}

#endif // OPERATION_STATUS_H
