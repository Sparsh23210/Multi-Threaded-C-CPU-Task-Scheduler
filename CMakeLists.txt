cmake_minimum_required(VERSION 3.10)
project(CPUScheduler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Threading library
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/scheduler.cpp
    src/metrics.cpp
    src/logger.cpp
)

# Executable target
add_executable(scheduler ${SOURCES})

# Target linking
target_link_libraries(scheduler PRIVATE Threads::Threads)
