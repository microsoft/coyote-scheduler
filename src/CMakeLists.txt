include_directories("../include/coyote")

set(src_files "ffi.cc")

add_library(coyote SHARED ${src_files})
set_target_properties(coyote PROPERTIES
    OUTPUT_NAME "coyote"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/")
target_compile_definitions(coyote PUBLIC COYOTE_SHARED COYOTE_EXPORT)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(coyote PRIVATE COYOTE_DEBUG_LOG)
endif()

add_library(coyote_static STATIC ${src_files})
set_target_properties(coyote_static PROPERTIES
    OUTPUT_NAME "coyote_static"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/")
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(coyote_static PRIVATE COYOTE_DEBUG_LOG)
endif()
