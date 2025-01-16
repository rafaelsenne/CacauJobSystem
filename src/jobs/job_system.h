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
            explicit job_system(size_t thread_count);
            ~job_system();

            /**
             * @brief Submits a job for execution
             * @param new_job The job to be executed
             * @details Distributes jobs across worker threads using round-robin
             */
            void submit(job* new_job);

            /**
             * @brief Submits a job that depends on other jobs
             * @param new_job The job to be executed
             * @param dependencies List of jobs that must complete before this one starts
             */
            void submit_with_dependencies(job* new_job, const std::vector<job *> &dependencies);

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
            void pause() { m_paused = true; }

            /**
             * @brief Resumes job execution
             */
            void resume() { m_paused = false; }

            void wait(job* job_to_wait_for);

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
            void worker_thread(size_t thread_index);

            /**
             * @brief Attempts to steal a job from another thread's queue
             * @param thread_index Index of the stealing thread
             * @param stolen_job Output parameter for the stolen job
             * @return true if a job was successfully stolen
             */
            bool steal_job(size_t thread_index, job *&stolen_job);

            // Thread management
            int mNextThread = 0;
            std::vector<std::thread> m_threads;
            std::vector<std::deque<job *>> m_thread_queues;
            std::vector<std::mutex> m_queue_mutexes;
            std::condition_variable m_condition;
            std::mutex m_global_mutex;
            bool m_stop;

            // Job tracking
            std::atomic<bool> m_paused{true};
            std::atomic<size_t> m_total_jobs{0};
            std::atomic<size_t> m_completed_jobs{0};
            //double m_total_execution_time{0.0};
            std::mutex execution_time_mutex;
            std::vector<job *> m_jobs_waiting_for_dependencies;

            // Performance monitoring
            std::vector<std::mutex> m_profiling_mutexes;
            std::vector<std::atomic<double>> m_thread_active_times;
            std::vector<std::atomic<double>> m_thread_idle_times;

        };

    } // namespace cacau::jobs

} // namespace cacau
