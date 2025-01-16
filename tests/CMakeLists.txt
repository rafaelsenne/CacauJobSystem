cmake_minimum_required(VERSION 3.10)

project(JobSystemTests)

# Define source directories
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# Enable testing
enable_testing()

# Add executables for each test
add_executable(TestScheduler ${TEST_DIR}/test_scheduler.cpp)
target_link_libraries(TestScheduler PRIVATE cacau_jobs)

add_executable(TestBenchmark ${TEST_DIR}/test_benchmark.cpp)
target_link_libraries(TestBenchmark PRIVATE cacau_jobs)

add_executable(TestStress ${TEST_DIR}/test_stress.cpp)
target_link_libraries(TestStress PRIVATE cacau_jobs)

# Add each test to ctest
add_test(NAME SchedulerTest COMMAND TestScheduler)
add_test(NAME BenchmarkTest COMMAND TestBenchmark)
add_test(NAME StressTest COMMAND TestStress)