#pragma once
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include "job.h"

namespace cacau
{
    namespace jobs
    {

        /**
         * @brief Multi-threaded job system that manages job execution and dependencies
         * @details Provides work stealing, dependency tracking, and performance monitoring
         *          Uses a thread pool to efficiently process jobs in parallel
         */
        class job_system
        {
        public:
            /**
             * @brief Initializes the job system with a specified number of worker threads
             * @param thread_count Number of worker threads to create in the thread pool
             */
            explicit job_system(size_t pThreadCount);
            ~job_system();

            /**
             * @brief Submits a job for execution
             * @param new_job The job to be executed
             * @details Distributes jobs across worker threads using round-robin
             */
            void submit(job* pNewJob);

            /**
             * @brief Submits a job that depends on other jobs
             * @param new_job The job to be executed
             * @param dependencies List of jobs that must complete before this one starts
             */
            void submit_with_dependencies(job* pNewJob, const std::vector<job*> &pDependencies);

            /**
             * @brief Gets the number of jobs waiting to be executed
             * @return Total number of pending jobs across all queues
             */
            size_t get_pending_jobs();

            /**
             * @brief Blocks until all submitted jobs are completed
             */
            void wait_for_all_jobs();

            /**
             * @brief Temporarily stops job execution
             */
            void pause() { mJobSystemPaused = true; }

            /**
             * @brief Resumes job execution
             */
            void resume() { mJobSystemPaused = false; }

            void wait(job* pJobToWait);

            /**
             * @brief Prints performance statistics for each worker thread
             * @details Shows the percentage of time each thread spent active vs idle
             */
            void print_thread_utilization() const;

        private:
            /**
             * @brief Main worker thread function that processes jobs
             * @param thread_index Identifier for the worker thread
             */
            void worker_thread(size_t pThreadIndex);

            /**
             * @brief Attempts to steal a job from another thread's queue
             * @param thread_index Index of the stealing thread
             * @param stolen_job Output parameter for the stolen job
             * @return true if a job was successfully stolen
             */
            bool steal_job(size_t pThreadIndex, job* &pStolenJob);

            // Thread management
            int mNextThread = 0;
            std::vector<std::thread> mThreads;
            std::vector<std::deque<job *>> mThreadQueues;
            std::vector<std::mutex> mQueueMutexes;
            std::condition_variable mCondition;
            std::mutex mGlobalMutex;
            bool mStop;

            // Job tracking
            std::atomic<bool> mJobSystemPaused{true};
            std::atomic<size_t> mTotalJobs{0};
            std::atomic<size_t> mCompletedJobs{0};
            //double m_total_execution_time{0.0};
            std::mutex mExecutionTimeMutex;
            std::vector<job *> mJobsWaitingForDependencies;

            // Performance monitoring
            std::vector<std::mutex> mProfilingMutexes;
            std::vector<std::atomic<double>> mThreadActiveTimes;
            std::vector<std::atomic<double>> mThreadIdleTimes;

        };

    } // namespace cacau::jobs

} // namespace cacau
