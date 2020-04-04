cmake_minimum_required (VERSION 3.8)
project ("coyote" CXX)

set(CMAKE_CXX_STANDARD 14)

enable_testing()

if(MSVC)
  add_compile_options(/permissive-)
  add_compile_options(/utf-8)
endif()

add_subdirectory(src/scheduler)
