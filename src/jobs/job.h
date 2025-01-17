#pragma once
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <vector>

#ifdef CACAU_DEBUG
#include <time.h>
#endif

namespace cacau
{
    namespace jobs
    {

    // Define CACAU_DEBUG to enable debug logging
    #ifdef CACAU_DEBUG
        static std::mutex mLogMutex;
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
                std::lock_guard<std::mutex> lock(mLogMutex); \
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
        explicit job(job_function pFunction, const char* pName = "UnamedJob")
            : mFunction(std::move(pFunction))
            , mRemainingDependencies(0)
            , mName(pName) {}

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
        bool add_dependant(job* pDependant);

        /**
         * @brief Adds this job as dependent on another job
         * @param dependency The job this one depends on
         */
        void add_dependency(job* pDependency);

        /**
         * @brief Called when a dependency completes
         * @param caller Name of the completed dependency (for logging)
         */
        void resolve_dependency(const char* pCallSource);

        /**
         * @brief Sets a callback for when all dependencies are resolved
         * @param callback Function to call when job becomes ready
         */
        void set_on_ready_callback(const std::function<void()>& pCallback);

        // Status checks
        bool is_ready() const { return mRemainingDependencies.load(std::memory_order_relaxed) == 0; }
        bool is_finished() const { return mIsFinished; }
        const char* name() const { return mName; }

    private:
        job_function mFunction;                    ///< The actual work to be performed
        std::atomic<int> mRemainingDependencies;  ///< Counter for unfinished dependencies
        std::function<void()> mOnReady;          ///< Callback for when job becomes ready
        std::vector<job*> mDependants;            ///< Jobs that depend on this one
        std::mutex mDependantsMutex;             ///< Protects access to dependants list
        bool mIsFinished = false;                  ///< Indicates if job has completed
        const char* mName;                        ///< Job identifier
    };

    } // namespace jobs
} // namespace cacau