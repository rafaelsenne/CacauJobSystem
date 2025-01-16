# Job System

Cacau Job System is a very simple multi-threaded job system that manages job execution and dependencies that I did for my custom game engine for fun. It provides work stealing, dependency tracking, and performance monitoring using a thread pool to efficiently process jobs in parallel. This system is intended to be used in a custom game engine called Cacau Engine.

## Features

- Multi-threaded job execution
- Dependency tracking and resolution
- Work stealing for load balancing
- Performance monitoring and thread utilization statistics

## Getting Started

### Prerequisites

- CMake 3.10 or higher
- C++11 compatible compiler

### Building the Project

You can just copy the ./src folder and include cacau_jobs.h

Or you can build the static library:

1. Clone the repository:
    ```sh
    git clone https://github.com/yourusername/cacau_jobs.git
    cd cacau_jobs
    ```

2. Create a build directory and navigate to it:
    ```sh
    mkdir build
    cd build
    ```

3. Configure the project using CMake:
    ```sh
    cmake ..
    ```

4. Build the project:
    ```sh
    cmake --build .
    ```

### Usage

#### Example: Submitting a Job

```cpp
#include "cacau_jobs.h"

void example_job() {
    std::cout << "Example job executed\n";
}

int main() {
    size_t numberOfThreads = 4;
    cacau::jobs::job_system jobSystem(numberOfThreads);
    cacau::jobs::job* test_job = new cacau::jobs::job(example_job, "ExampleJob");

    jobSystem.submit(test_job);
    jobSystem.wait_for_all_jobs();

    return 0;
}
```

#### Example: Job Dependencies

```cpp
#include "cacau_jobs.h"

void job_example1() {
    std::cout << "Job 1 executed\n";
}

void job_example2() {
    std::cout << "Job 2 executed\n";
}

void job_example3() {
    std::cout << "Job 3 executed\n";
}

int main() {
    size_t numberOfThreads = 4;
    cacau::jobs::job_system jobSystem(numberOfThreads);
    cacau::jobs::job* job1 = new cacau::jobs::job(job_example1, "Job1");
    cacau::jobs::job* job2 = new cacau::jobs::job(job_example2, "Job2");
    cacau::jobs::job* job3 = new cacau::jobs::job(job_example3, "Job3");

    // Submits job3 that depends on both job1 and job2 to be finished before starting
    jobSystem.submit_with_dependencies(job3, {job1, job2});
    
    jobSystem.submit(job1);
    jobSystem.submit(job2);
    
    // Wait for job3, which will only start after job1 and job2 is finished
    jobSystem.wait(job3);

    // Or wait for all jobs
    // job_system.wait_for_all_jobs();

    return 0;
}
```

## Feature List

### Features Already Working

- [x] Multi-threaded job execution
- [x] Dependency tracking and resolution
- [x] Work stealing for load balancing
- [x] Performance monitoring and thread utilization statistics
- [ ] Job grouping: Allow grouping of jobs to be executed together.
- [ ] Job prioritization: Implement a priority system for jobs to ensure critical tasks are executed first.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.