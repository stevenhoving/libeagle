cmake_minimum_required (VERSION 3.13)

project(eagle
    VERSION 0.0.1
    LANGUAGES CXX
)

# compiler part
set(CMAKE_CXX_STANDARD 20)

# project part
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_executable(eagle
    main.cpp
    eagle.cpp
    eagle.h
    stream_buffer.h
)

target_include_directories(eagle
  PUBLIC
    .
)

set_target_properties(eagle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$(Configuration)
)
