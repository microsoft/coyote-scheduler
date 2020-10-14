// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ffi.h"
#include "scheduler.h"

using namespace coyote;

extern "C"
{
    COYOTE_API void* create_scheduler()
    {
        return new Scheduler();
    }

    COYOTE_API void* create_scheduler_with_random_strategy(size_t seed)
    {
        auto settings = std::make_unique<Settings>();
        settings->use_random_strategy(seed);
        return new Scheduler(std::move(settings));
    }

    COYOTE_API int attach(void* scheduler)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->attach();
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int detach(void* scheduler)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->detach();
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int create_operation(void* scheduler, size_t operation_id)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->create_operation(operation_id);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int start_operation(void* scheduler, size_t operation_id)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->start_operation(operation_id);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int join_operation(void* scheduler, size_t operation_id)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->join_operation(operation_id);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int join_operations(void* scheduler, size_t* operation_ids, size_t size, bool wait_all)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->join_operations(operation_ids, size, wait_all);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int complete_operation(void* scheduler, size_t operation_id)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->complete_operation(operation_id);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int create_resource(void* scheduler, size_t resource_id)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->create_resource(resource_id);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int wait_resource(void* scheduler, size_t resource_id)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->wait_resource(resource_id);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int wait_resources(void* scheduler, size_t* resource_ids, size_t size, bool wait_all)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->wait_resources(resource_ids, size, wait_all);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int signal_resource(void* scheduler, size_t resource_id)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->signal_resource(resource_id);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int delete_resource(void* scheduler, size_t resource_id)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->delete_resource(resource_id);
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int schedule_next(void* scheduler)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->schedule_next();
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API bool next_boolean(void* scheduler)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        return ptr->next_boolean();
    }

    COYOTE_API size_t next_integer(void* scheduler, size_t max_value)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        return ptr->next_integer(max_value);
    }

    COYOTE_API size_t seed(void* scheduler)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        return ptr->seed();
    }

    COYOTE_API int error_code(void* scheduler)
    {
        Scheduler* ptr = (Scheduler*)scheduler;
        ErrorCode error_code = ptr->error_code();
        return static_cast<std::underlying_type_t<ErrorCode>>(error_code);
    }

    COYOTE_API int dispose_scheduler(void* scheduler)
    {
        try
        {
            Scheduler* ptr = (Scheduler*)scheduler;
            delete ptr;
        }
        catch (...)
        {
            return static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::Failure);
        }

        return static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::Success);
    }
}
