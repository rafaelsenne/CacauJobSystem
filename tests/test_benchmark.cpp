#include <iostream>
#include <chrono>
#include "cacau_jobs.h"

void compute_sum_of_squares(size_t pStart, size_t pEnd)
{
    volatile double result = 0.0;
    for (size_t i = pStart; i <= pEnd; ++i)
    {
        result += i * i;
    }
}

int execute_benchmark(size_t pThreads, size_t pJobs)
{
    cacau::jobs::job_system jobSystem(pThreads);

    auto benchmarkStart = std::chrono::high_resolution_clock::now();
    jobSystem.pause();

    size_t step = 20000;

    for (size_t i = 0; i < pJobs; ++i)
    {
        cacau::jobs::job *newJob = new cacau::jobs::job([i, step]
                                                        {
            size_t range_start = i * step;
            size_t range_end = (i + 1) * step - 1;
            compute_sum_of_squares(range_start, range_end); });
        jobSystem.submit(newJob);
    }

    auto submissionTime = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - benchmarkStart);

    //job_system.resume();
    jobSystem.wait_for_all_jobs();

    auto benchmarkEnd = std::chrono::high_resolution_clock::now();

    double executionDuration = std::chrono::duration<double, std::milli>(
                                    benchmarkEnd - benchmarkStart - submissionTime)
                                    .count();
    double totalDuration = std::chrono::duration<double, std::milli>(
                                benchmarkEnd - benchmarkStart)
                                .count();

    std::cout << "Threads: " << pThreads << "\n"
              << "Jobs Submitted: " << pJobs << "\n"
              << "Execution Time: " << executionDuration << " ms\n"
              << "Total Time: " << totalDuration << " ms\n";
    
    std::cout << std::endl;
    jobSystem.print_thread_utilization();

    return 0;
}

int main()
{
    execute_benchmark(16, 1000000);
    execute_benchmark(8, 1000000);
    execute_benchmark(4, 1000000);
    execute_benchmark(2, 1000000);

    return 0;
}
