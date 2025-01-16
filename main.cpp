#include <iostream>
#include <chrono>
#include <thread>
#include "cacau_jobs.h"

/**
 * @brief Helper function that performs CPU-intensive work for benchmarking
 * @param start Starting number in the range
 * @param end Ending number in the range
 */
void compute_sum_of_squares(size_t pStart, size_t pEnd) {
    volatile double result = 0.0;
    for (size_t i = pStart; i <= pEnd; ++i) {
        result += i * i;
    }
}

void example_job()
{
    std::cout << "Example job executed\n";
}

/**
 * @brief Creates a test scenario with interdependent jobs
 * @param job_system Reference to the job system to use
 */
void test_job_scheduler_runner(cacau::jobs::job_system& pJobSystem) {
    pJobSystem.resume();
    
    // Create all jobs upfront with descriptive names
    auto job1 = new cacau::jobs::job([]{ std::cout << "Calling Job 1\n"; }, "Job 1");
    auto job2 = new cacau::jobs::job([]{ std::cout << "Calling Job 2\n"; }, "Job 2");
    
    auto job3 = new cacau::jobs::job([]
        { 
            std::cout << "Calling Job 3\n"; 
            //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }, "Job 3");

    auto job4 = new cacau::jobs::job([]{ std::cout << "Calling Job 4\n"; }, "Job 4");
    auto job5 = new cacau::jobs::job([]{ std::cout << "Calling Job 5\n"; }, "Job 5");
    auto job6 = new cacau::jobs::job([]{ std::cout << "Calling Job 6\n"; }, "Job 6");
    auto job7 = new cacau::jobs::job([]{ std::cout << "Calling Job 7\n"; }, "Job 7");
    auto job8 = new cacau::jobs::job(example_job, "ExampleJob");

    // Submit jobs with dependencies in a specific order
    pJobSystem.submit_with_dependencies(job6, {job3, job4});  // Job 6 depends on 3 and 4
    pJobSystem.submit_with_dependencies(job5, {job3, job4});  // Job 5 depends on 3 and 4
    pJobSystem.submit_with_dependencies(job4, {job3});        // Job 4 depends on 3
    pJobSystem.submit_with_dependencies(job7, {job1, job2});  // Job 7 depends on 1 and 2
    
    // Submit independent jobs first
    pJobSystem.submit(job1);
    pJobSystem.submit(job2);
    pJobSystem.submit(job8);

    // Submit final dependent job
    pJobSystem.submit_with_dependencies(job3, {job1, job2});  // Job 3 depends on 1 and 2

    pJobSystem.wait(job3);
}

/**
 * @brief Performance benchmark test for the job system
 * Creates and executes a large number of CPU-intensive jobs
 */
int test_stress(size_t pThreadCount, size_t pJobCount) {

    cacau::jobs::job_system jobSystem(pThreadCount);
    
    // Start benchmark timing
    auto benchmarkStart = std::chrono::high_resolution_clock::now();
    jobSystem.pause();

    // Create and submit benchmark jobs
    size_t step = 20000;
    for (size_t i = 0; i < pJobCount; ++i) {
        cacau::jobs::job *newJob = new cacau::jobs::job([i,step]
                                                        {
            size_t rangeStart = i * step;
            size_t rangeEnd = (i + 1) * step - 1;
            compute_sum_of_squares(rangeStart, rangeEnd); 
            });
                                   jobSystem.submit(newJob);
    }

    // Calculate submission time
    auto submissionTime = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - benchmarkStart);
    
    // Execute jobs and wait for completion
    jobSystem.resume();
    std::cout << "Submission Time: " << submissionTime.count() << " ms\n";
    while (jobSystem.get_pending_jobs() > 0)
    {        
        std::cout << "Waiting for all jobs to finish... " << jobSystem.get_pending_jobs() << " jobs left\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
    }
    
    // Wait for all jobs to finish
    jobSystem.wait_for_all_jobs();
    std::cout << "All jobs finished.\n";

    // Print thread utilization in %
    jobSystem.print_thread_utilization();
    auto benchmarkEnd = std::chrono::high_resolution_clock::now();
    
    double executionDuration = std::chrono::duration<double, std::milli>(
        benchmarkEnd - benchmarkStart - submissionTime).count();
    double totalDuration = std::chrono::duration<double, std::milli>(
        benchmarkEnd - benchmarkStart).count();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    // Print benchmark results
    std::cout << "Benchmark Results:\n"
              << "Threads: " << pThreadCount << "\n"
              << "Jobs Submitted: " << pJobCount << "\n"
              << "Excecution Time: " << executionDuration << " ms\n"
              << "Total Time: " << totalDuration << " ms\n";

    std::cout << std::endl;

    return 0;
}

/**
 * @brief Stress test for the job system
 * Repeatedly creates and executes job dependency scenarios
 */
int test_job_scheduler(size_t pThreadCount, size_t pJobCount) 
{
    cacau::jobs::job_system jobSystem(pThreadCount);
    
    // Pause to submit jobs (not actually needed, this is for testing purposes)
    jobSystem.pause();
    
    // Run multiple scheduler tests
    for (size_t i = 0; i < pJobCount; i++)
    {
        test_job_scheduler_runner(jobSystem);
    }

    // Wait for all jobs to finish
    jobSystem.wait_for_all_jobs();

    // Print thread utilization in %
    jobSystem.print_thread_utilization();
    std::cout << "All jobs completed.\n";

    return 0;
}

/**
 * @brief Entry point for the example program
 * Runs a series of stress tests for the job system, including a job scheduler test
 * and a stress test with different thread counts
 * @return 0 on success
 */
int main(int argc, char **argv) {
    test_job_scheduler(16, 10000);
    test_job_scheduler(8, 10000);
    test_job_scheduler(4, 10000);
    test_stress(16, 1000000);
    test_stress(8, 1000000);
    test_stress(4, 1000000);
    return 0;
}