#pragma once
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <vector>

namespace cacau
{
namespace jobs
{

// Define CACAU_DEBUG to enable debug logging
#ifdef CACAU_DEBUG
    inline std::mutex log_mutex;
    #define LOG_MESSAGE(message) \
        do { \
            auto my_now = std::chrono::system_clock::now(); \
            auto my_time = std::chrono::system_clock::to_time_t(my_now); \
            auto my_ms = std::chrono::duration_cast<std::chrono::milliseconds>( \
                            my_now.time_since_epoch()) % 1000; \
            std::ostringstream my_stream; \
            my_stream << "[" << std::put_time(std::localtime(&my_time), "%H:%M:%S") << "." \
                     << std::setw(3) << std::setfill('0') << my_ms.count() \
                     << "][" << std::this_thread::get_id() << "] " << message; \
            std::lock_guard<std::mutex> lock(log_mutex); \
            std::cout << my_stream.str() << "\n"; \
        } while(0)
#else
    #define LOG_MESSAGE(message) //do {} while(0)
#endif

/**
 * @brief A job unit that can be executed by the job system
 * @details Supports dependency tracking, execution callbacks, and thread-safe operations
 */
class job
{
public:
    using job_function = std::function<void()>;

    /**
     * @brief Constructs a new job with a function to execute
     * @param func The function to be executed when the job runs
     * @param pName Identifier for the job (used in logging)
     */
    explicit job(job_function func, const char* pName = "UnamedJob")
        : m_function(std::move(func))
        , m_remaining_dependencies(0)
        , m_name(pName) {}

    /**
     * @brief Executes the job's function and notifies dependent jobs
     * @details Thread-safe execution that handles dependency resolution
     */
    void execute();

    /**
     * @brief Adds a job that depends on this job's completion
     * @param dependant The job that depends on this one
     * @return true if dependency was added, false if this job is already finished
     */
    bool add_dependant(job* dependant);

    /**
     * @brief Adds this job as dependent on another job
     * @param dependency The job this one depends on
     */
    void add_dependency(job* dependency);

    /**
     * @brief Called when a dependency completes
     * @param caller Name of the completed dependency (for logging)
     */
    void resolve_dependency(const char* caller);

    /**
     * @brief Sets a callback for when all dependencies are resolved
     * @param callback Function to call when job becomes ready
     */
    void set_on_ready_callback(const std::function<void()>& callback);

    // Status checks
    bool is_ready() const { return m_remaining_dependencies.load(std::memory_order_relaxed) == 0; }
    bool is_finished() const { return mIsFinished; }
    const char* name() const { return m_name; }

private:
    job_function m_function;                    ///< The actual work to be performed
    std::atomic<int> m_remaining_dependencies;  ///< Counter for unfinished dependencies
    std::function<void()> m_on_ready;          ///< Callback for when job becomes ready
    std::vector<job*> m_dependants;            ///< Jobs that depend on this one
    std::mutex m_dependants_mutex;             ///< Protects access to dependants list
    bool mIsFinished = false;                  ///< Indicates if job has completed
    const char* m_name;                        ///< Job identifier
};

} // namespace jobs
} // namespace cacau