#include "job_system.h"
#include <mutex>

namespace cacau
{
    namespace jobs
    {
    std::mutex execution_time_mutex;

    job_system::job_system(size_t thread_count)
        : 
        mNextThread(0),
        m_threads(),
        m_thread_queues(thread_count),
        m_queue_mutexes(thread_count),
        m_condition(),
        m_global_mutex(),
        m_stop(false),
        m_paused(true),
        m_total_jobs(0),
        m_completed_jobs(0),
        m_jobs_waiting_for_dependencies(),
        m_profiling_mutexes(thread_count),
        m_thread_active_times(thread_count),
        m_thread_idle_times(thread_count)
    {
        for (size_t i = 0; i < thread_count; ++i)
        {
            m_threads.emplace_back([this, i]
                                   { worker_thread(i); });
            m_thread_active_times[i] = 0.0;
            m_thread_idle_times[i] = 0.0;
        }
    }


    job_system::~job_system()
    {
        {
            std::unique_lock<std::mutex> lock(m_global_mutex);
            m_stop = true;
        }
        m_condition.notify_all();

        for (auto &thread : m_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    void job_system::submit(job* new_job)
    {
        {
            std::unique_lock<std::mutex> lock(m_queue_mutexes[mNextThread]);
            m_thread_queues[mNextThread].push_back(new_job);
            ++m_total_jobs;
        }
        
        m_condition.notify_all();

        mNextThread = (mNextThread + 1) % m_thread_queues.size(); // Round-robin distribution
    }

    void job_system::submit_with_dependencies(job* new_job, const std::vector<job *> &dependencies)
    {
        // Handle jobs with no dependencies
        if (dependencies.empty())
        {
            LOG_MESSAGE("Submitting " + std::string(new_job.name()) + " with no dependencies");
            submit(new_job);
            return;
        }

        // Register job as waiting for dependencies
        LOG_MESSAGE("Submitting " + std::string(new_job.name()) +
                    " with " + std::to_string(dependencies.size()) + " dependencies");
        {
            std::lock_guard<std::mutex> lock(m_global_mutex);
            m_jobs_waiting_for_dependencies.push_back(new_job);
        }

        // Add dependencies and track if any are still pending
        bool hasPendingDependencies = false;
        for (auto *dependency : dependencies)
        {
            if(dependency->add_dependant(new_job))
            {
                hasPendingDependencies = true;
            }
        }

        // If all dependencies are already satisfied, submit the job directly
        if (!hasPendingDependencies)
        {
            LOG_MESSAGE("Will execute " + std::string(new_job.name()) +
                        " as all dependencies are already satisfied");
            m_jobs_waiting_for_dependencies.erase(
                std::remove(m_jobs_waiting_for_dependencies.begin(), 
                           m_jobs_waiting_for_dependencies.end(), new_job), 
                m_jobs_waiting_for_dependencies.end());
            submit(new_job);
        }
    }

    bool job_system::steal_job(size_t thread_index, job *&stolen_job)
    {
        // Try to steal from other threads' queues
        for (size_t i = 0; i < m_thread_queues.size(); ++i)
        {
            if (i == thread_index)
                continue; // Skip own queue

            // Try to acquire lock and steal a job
            std::unique_lock<std::mutex> lock(m_queue_mutexes[i]);
            if (!m_thread_queues[i].empty())
            {
                stolen_job = m_thread_queues[i].front();
                m_thread_queues[i].pop_front();
                return true;
            }
        }
        return false;
    }

    void job_system::worker_thread(size_t thread_index)
    {
        //auto thread_start = std::chrono::high_resolution_clock::now();

        while (true)
        {
            // Check if system is paused
            if (m_paused)
            {
                std::this_thread::yield();
                continue;
            }

            job *my_job = nullptr;
            auto idle_start = std::chrono::high_resolution_clock::now();

            // Try to get job from local queue
            {
                std::unique_lock<std::mutex> lock(m_queue_mutexes[thread_index]);
                if (!m_thread_queues[thread_index].empty())
                {
                    my_job = m_thread_queues[thread_index].front();
                    m_thread_queues[thread_index].pop_front();
                }
            }

            // If no local job, try to steal one
            if (!my_job && !steal_job(thread_index, my_job))
            {
                // Update idle time statistics
                auto idle_end = std::chrono::high_resolution_clock::now();
                double idle_time = std::chrono::duration<double, std::milli>(
                    idle_end - idle_start).count();
                double current_idle_time = m_thread_idle_times[thread_index].load(
                    std::memory_order_relaxed);
                
                {
                    std::lock_guard<std::mutex> lock(m_profiling_mutexes[thread_index]);
                    m_thread_idle_times[thread_index].store(
                        current_idle_time + idle_time, std::memory_order_relaxed);
                }

                // Wait for new work or shutdown signal
                std::unique_lock<std::mutex> lock(m_global_mutex);
                m_condition.wait(lock, [this] {
                    return m_stop || (!m_paused && m_total_jobs > m_completed_jobs);
                });

                // Check if should exit
                if (m_stop && m_total_jobs == m_completed_jobs)
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
                double currentActiveTime = m_thread_active_times[thread_index].load(
                    std::memory_order_relaxed);
                
                {
                    std::lock_guard<std::mutex> lock(m_profiling_mutexes[thread_index]);
                    m_thread_active_times[thread_index].store(
                        currentActiveTime + execution_time, std::memory_order_relaxed);
                }

                ++m_completed_jobs;
                delete my_job;
            }
        }
    }

    size_t job_system::get_pending_jobs()
    {
        size_t pending_jobs = 0;

        {
            // Count jobs in thread queues
            for (size_t i = 0; i < m_thread_queues.size(); ++i)
            {
                std::unique_lock<std::mutex> lock(m_queue_mutexes[i]);                
                pending_jobs += m_thread_queues[i].size();
            }
        }

        // Add jobs waiting for dependencies
        {
            std::lock_guard<std::mutex> lock(m_global_mutex); // Protect access to jobs with dependencies
            for (const auto &job : m_jobs_waiting_for_dependencies)
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

    void job_system::wait(job* job_to_wait_for)
    {
        if(job_to_wait_for == nullptr)
        {
            std::cerr << "Error: job to wait is null\n";
            return;
        }

        resume();
        while (job_to_wait_for != nullptr && !job_to_wait_for->is_finished())
        {
            std::this_thread::yield();
        }

#ifdef CACAU_DEBUG
        {
            std::lock_guard<std::mutex> lock(m_global_mutex);
            std::cout << "Job " << job_to_wait_for.name() << " finished\n";
        }
#endif
    }

    void job_system::print_thread_utilization() const
    {
        for (size_t i = 0; i < m_threads.size(); ++i) {
            double total_time = m_thread_active_times[i].load() + m_thread_idle_times[i].load();
            double active_percentage = (total_time > 0) ? 
                (m_thread_active_times[i].load() / total_time) * 100.0 : 0.0;
            double idle_percentage = 100.0 - active_percentage;

            std::cout << "Thread " << i << ": "
                      << "Active: " << active_percentage << "%, "
                      << "Idle: " << idle_percentage << "%\n";
        }
    }

    } // namespace jobs
} // namespace cacau