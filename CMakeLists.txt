cmake_minimum_required(VERSION 3.10)
project(cacau_jobs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recursively get all source files in the src directory
file(GLOB_RECURSE SOURCES src/*.cpp)

set(CMAKE_OBJECT_PATH_MAX 260) # For long paths on Windows
set(PLATFORM_TARGET ${CMAKE_SYSTEM_NAME}-$<IF:$<BOOL:${CMAKE_GENERATOR_PLATFORM}>,${CMAKE_GENERATOR_PLATFORM},$<PLATFORM_ID>>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${PLATFORM_TARGET}/${PROJECT_NAME}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${PLATFORM_TARGET}/${PROJECT_NAME}/lib/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${PLATFORM_TARGET}/${PROJECT_NAME}/lib/")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${PLATFORM_TARGET}/${PROJECT_NAME}/debug/")

# Options
set(ENABLE_CACAU_TESTS "Enable tests" ON)

# Add library
add_library(cacau_jobs STATIC)
target_sources(cacau_jobs PRIVATE ${SOURCES})
target_include_directories(cacau_jobs PUBLIC src/)

# Link libraries
target_link_libraries(cacau_jobs PUBLIC)
target_link_libraries(cacau_jobs PRIVATE)

# Add test units
if(ENABLE_CACAU_TESTS)
    # Add executable for testing
    add_executable(CacauJobsTest main.cpp)
    target_link_libraries(CacauJobsTest PRIVATE cacau_jobs)
    target_include_directories(CacauJobsTest PRIVATE src/)
    enable_testing()
    add_subdirectory(tests)
endif()