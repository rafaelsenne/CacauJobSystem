#include <iostream>
#include "cacau_jobs.h"

void compute_sum_of_squares(size_t start, size_t end)
{
    volatile double result = 0.0;
    for (size_t i = start; i <= end; ++i)
    {
        result += i * i;
    }
}

void TestJobScheduler(cacau::jobs::job_system &job_system)
{
    auto *job1 = new cacau::jobs::job([]
                                      { std::cout << "Calling Job 1\n"; }, "Job 1");
    auto *job2 = new cacau::jobs::job([]
                                      { std::cout << "Calling Job 2\n"; }, "Job 2");
    auto *job3 = new cacau::jobs::job([]
                                      { std::cout << "Calling Job 3\n"; }, "Job 3");
    auto *job4 = new cacau::jobs::job([]
                                      { std::cout << "Calling Job 4\n"; }, "Job 4");
    auto *job5 = new cacau::jobs::job([]
                                      { std::cout << "Calling Job 5\n"; }, "Job 5");
    auto *job6 = new cacau::jobs::job([]
                                      { std::cout << "Calling Job 6\n"; }, "Job 6");
    auto *job7 = new cacau::jobs::job([]
                                      { std::cout << "Calling Job 7\n"; }, "Job 7");

    job_system.submit_with_dependencies(job6, {job3, job4});
    job_system.submit_with_dependencies(job5, {job3, job4});
    job_system.submit_with_dependencies(job4, {job3});
    job_system.submit_with_dependencies(job7, {job1, job2});

    job_system.submit(job1);
    job_system.submit(job2);
    job_system.submit_with_dependencies(job3, {job1, job2});
}

int main()
{
    constexpr size_t thread_count = 16;
    cacau::jobs::job_system job_system(thread_count);

    job_system.pause();

    for (size_t i = 0; i < 1000; i++)
    {
        TestJobScheduler(job_system);
    }

    job_system.wait_for_all_jobs();
    std::cout << "All jobs completed.\n";

    return 0;
}
