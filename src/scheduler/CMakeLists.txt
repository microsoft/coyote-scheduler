include_directories(.)

set(src_files
    "ffi.cc"
    "scheduler.cc"
    "errors/error_code.cc"
    "operations/operation.cc")

add_library(coyote SHARED ${src_files})
set_target_properties(coyote PROPERTIES
    OUTPUT_NAME "coyote"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
target_compile_definitions(coyote PUBLIC COYOTE_SHARED COYOTE_EXPORT)

add_library(coyote_static STATIC ${src_files})
set_target_properties(coyote_static PROPERTIES
    OUTPUT_NAME "coyote_static"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")

if(CMAKE_TESTING_ENABLED)
    add_subdirectory(test)
endif()
