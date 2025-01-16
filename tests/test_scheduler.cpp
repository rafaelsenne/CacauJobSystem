#include <iostream>
#include "cacau_jobs.h"

void test_job_scheduler_runner(cacau::jobs::job_system &jobSystem)
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

    jobSystem.submit_with_dependencies(job6, {job3, job4});
    jobSystem.submit_with_dependencies(job5, {job3, job4});
    jobSystem.submit_with_dependencies(job4, {job3});
    jobSystem.submit_with_dependencies(job7, {job1, job2});

    jobSystem.submit(job1);
    jobSystem.submit(job2);
    jobSystem.submit_with_dependencies(job3, {job1, job2});
}

int main()
{
    std::cout << "Scheduler Test Started.\n";
    cacau::jobs::job_system job_system(4); // Example thread count
    test_job_scheduler_runner(job_system);
    job_system.wait_for_all_jobs();
    std::cout << "Scheduler Test Completed.\n";
    return 0;
}