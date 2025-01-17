#include "job_system.h"
#include <mutex>

namespace cacau
{
    namespace jobs
    {
    std::mutex execution_time_mutex;

    job_system::job_system(size_t pThreadCount)
        : 
        mNextThread(0),
        mThreads(),
        mThreadQueues(pThreadCount),
        mQueueMutexes(pThreadCount),
        mCondition(),
        mGlobalMutex(),
        mStop(false),
        mJobSystemPaused(true),
        mTotalJobs(0),
        mCompletedJobs(0),
        mJobsWaitingForDependencies(),
        mProfilingMutexes(pThreadCount),
        mThreadActiveTimes(pThreadCount),
        mThreadIdleTimes(pThreadCount)
    {
        for (size_t i = 0; i < pThreadCount; ++i)
        {
            mThreads.emplace_back([this, i]
                                   { worker_thread(i); });
            mThreadActiveTimes[i] = 0.0;
            mThreadIdleTimes[i] = 0.0;
        }
    }


    job_system::~job_system()
    {
        {
            std::unique_lock<std::mutex> lock(mGlobalMutex);
            mStop = true;
        }
        mCondition.notify_all();

        for (auto &thread : mThreads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    void job_system::submit(job* pNewJob)
    {
        {
            std::unique_lock<std::mutex> lock(mQueueMutexes[mNextThread]);
            mThreadQueues[mNextThread].push_back(pNewJob);
            ++mTotalJobs;
        }
        
        mCondition.notify_all();

        mNextThread = (mNextThread + 1) % mThreadQueues.size(); // Round-robin distribution
    }

    void job_system::submit_with_dependencies(job* pNewJob, const std::vector<job*> &pDependencies)
    {
        // Handle jobs with no dependencies
        if (pDependencies.empty())
        {
            LOG_MESSAGE("Submitting " + std::string(pNewJob->name()) + " with no dependencies");
            submit(pNewJob);
            return;
        }

        // Register job as waiting for dependencies
        LOG_MESSAGE("Submitting " + std::string(pNewJob->name()) +
                    " with " + std::to_string(pDependencies.size()) + " dependencies");
        {
            std::lock_guard<std::mutex> lock(mGlobalMutex);
            mJobsWaitingForDependencies.push_back(pNewJob);
        }

        // Add dependencies and track if any are still pending
        bool hasPendingDependencies = false;
        for (auto *dependency : pDependencies)
        {
            if(dependency->add_dependant(pNewJob))
            {
                hasPendingDependencies = true;
            }
        }

        // If all dependencies are already satisfied, submit the job directly
        if (!hasPendingDependencies)
        {
            LOG_MESSAGE("Will execute " + std::string(pNewJob->name()) +
                        " as all dependencies are already satisfied");
            mJobsWaitingForDependencies.erase(
                std::remove(mJobsWaitingForDependencies.begin(), 
                           mJobsWaitingForDependencies.end(), pNewJob), 
                mJobsWaitingForDependencies.end());
            submit(pNewJob);
        }
    }

    bool job_system::steal_job(size_t pThreadIndex, job* &pStolenJob)
    {
        // Try to steal from other threads' queues
        for (size_t i = 0; i < mThreadQueues.size(); ++i)
        {
            if (i == pThreadIndex)
                continue; // Skip own queue

            // Try to acquire lock and steal a job
            std::unique_lock<std::mutex> lock(mQueueMutexes[i]);
            if (!mThreadQueues[i].empty())
            {
                pStolenJob = mThreadQueues[i].front();
                mThreadQueues[i].pop_front();
                return true;
            }
        }
        return false;
    }

    void job_system::worker_thread(size_t pThreadIndex)
    {
        //auto thread_start = std::chrono::high_resolution_clock::now();

        while (true)
        {
            // Check if system is paused
            if (mJobSystemPaused)
            {
                std::this_thread::yield();
                continue;
            }

            job *my_job = nullptr;
            auto idle_start = std::chrono::high_resolution_clock::now();

            // Try to get job from local queue
            {
                std::unique_lock<std::mutex> lock(mQueueMutexes[pThreadIndex]);
                if (!mThreadQueues[pThreadIndex].empty())
                {
                    my_job = mThreadQueues[pThreadIndex].front();
                    mThreadQueues[pThreadIndex].pop_front();
                }
            }

            // If no local job, try to steal one
            if (!my_job && !steal_job(pThreadIndex, my_job))
            {
                // Update idle time statistics
                auto idle_end = std::chrono::high_resolution_clock::now();
                double idle_time = std::chrono::duration<double, std::milli>(
                    idle_end - idle_start).count();
                double current_idle_time = mThreadIdleTimes[pThreadIndex].load(
                    std::memory_order_relaxed);
                
                {
                    std::lock_guard<std::mutex> lock(mProfilingMutexes[pThreadIndex]);
                    mThreadIdleTimes[pThreadIndex].store(
                        current_idle_time + idle_time, std::memory_order_relaxed);
                }

                // Wait for new work or shutdown signal
                std::unique_lock<std::mutex> lock(mGlobalMutex);
                mCondition.wait(lock, [this] {
                    return mStop || (!mJobSystemPaused && mTotalJobs > mCompletedJobs);
                });

                // Check if should exit
                if (mStop && mTotalJobs == mCompletedJobs)
                {
                    return;
                }
                continue;
            }

            // Execute the job if we got one
            if (my_job)
            {
                // Track execution time for profiling
                auto start_time = std::chrono::high_resolution_clock::now();
                my_job->execute();
                auto end_time = std::chrono::high_resolution_clock::now();

                // Update active time statistics
                double execution_time = std::chrono::duration<double, std::milli>(
                    end_time - start_time).count();
                double currentActiveTime = mThreadActiveTimes[pThreadIndex].load(
                    std::memory_order_relaxed);
                
                {
                    std::lock_guard<std::mutex> lock(mProfilingMutexes[pThreadIndex]);
                    mThreadActiveTimes[pThreadIndex].store(
                        currentActiveTime + execution_time, std::memory_order_relaxed);
                }

                ++mCompletedJobs;
                delete my_job;
            }
        }
    }

    size_t job_system::get_pending_jobs()
    {
        size_t pending_jobs = 0;

        {
            // Count jobs in thread queues
            for (size_t i = 0; i < mThreadQueues.size(); ++i)
            {
                std::unique_lock<std::mutex> lock(mQueueMutexes[i]);                
                pending_jobs += mThreadQueues[i].size();
            }
        }

        // Add jobs waiting for dependencies
        {
            std::lock_guard<std::mutex> lock(mGlobalMutex); // Protect access to jobs with dependencies
            for (const auto &job : mJobsWaitingForDependencies)
            {
                if (job != nullptr && !job->is_ready())
                {
                    ++pending_jobs;
                }
            }
        }

        return pending_jobs;
    }

    void job_system::wait_for_all_jobs() {
        resume();

        while (get_pending_jobs() > 0) {
            std::this_thread::yield(); // Allow worker threads to run
        }
    }

    void job_system::wait(job* pJobToWait)
    {
        if(pJobToWait == nullptr)
        {
            std::cerr << "Error: job to wait is null\n";
            return;
        }

        resume();
        while (pJobToWait != nullptr && !pJobToWait->is_finished())
        {
            std::this_thread::yield();
        }

#ifdef CACAU_DEBUG
        {
            std::lock_guard<std::mutex> lock(mGlobalMutex);
            std::cout << "Job " << pJobToWait->name() << " finished\n";
        }
#endif
    }

    void job_system::print_thread_utilization() const
    {
        for (size_t i = 0; i < mThreads.size(); ++i) {
            double total_time = mThreadActiveTimes[i].load() + mThreadIdleTimes[i].load();
            double active_percentage = (total_time > 0) ? 
                (mThreadActiveTimes[i].load() / total_time) * 100.0 : 0.0;
            double idle_percentage = 100.0 - active_percentage;

            std::cout << "Thread " << i << ": "
                      << "Active: " << active_percentage << "%, "
                      << "Idle: " << idle_percentage << "%\n";
        }
    }

    } // namespace jobs
} // namespace cacau