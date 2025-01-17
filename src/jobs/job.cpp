#include "job.h"

namespace cacau 
{
    namespace jobs
    {

        void job::execute()
        {
            LOG_MESSAGE(std::string(mName) + " Executing job " + std::string(mName));
            if (mFunction)
            {
                mFunction();
            }
            mIsFinished = true;
            {
                std::lock_guard<std::mutex> lock(mDependantsMutex);
                auto dependants = mDependants;
                for (auto *dependant : dependants)
                {
                    if (dependant == nullptr)
                    {
                        LOG_MESSAGE(std::string(mName) + " Dependant job is null, skipping");
                        continue;
                    }
                    if (!dependant->is_ready())
                    {
                        dependant->resolve_dependency(mName);
                    }
                }
            }

            LOG_MESSAGE(std::string(mName) + " Exiting execute");
        }

        bool job::add_dependant(job *pDependant)
        {
            if (is_finished())
            {
                return false;
            }

            {
                std::lock_guard<std::mutex> lock(mDependantsMutex);
                LOG_MESSAGE(std::string(mName) + " Attempting to lock m_dependants_mutex");

                LOG_MESSAGE(std::string(mName) + " Adding dependant " + std::string(pDependant->mName));
                mDependants.push_back(pDependant);
                LOG_MESSAGE(std::string(mName) + " added dependant " + std::string(pDependant->mName));
                pDependant->add_dependency(this);
            }

            LOG_MESSAGE(std::string(mName) + " Exiting add_dependant");
            return true;
        }

        void job::add_dependency(job *pDependency)
        {
            mRemainingDependencies.fetch_add(1, std::memory_order_relaxed);
            LOG_MESSAGE(std::string(mName) + " Dependency added, remaining: " +
                        std::to_string(mRemainingDependencies.load()));
        }

        void job::resolve_dependency(const char *pCallSource)
        {
            int remaining = mRemainingDependencies.fetch_sub(1, std::memory_order_acq_rel) - 1;
            LOG_MESSAGE(std::string(mName) + " Resolving dependency [" + pCallSource +
                        "], remaining: " + std::to_string(remaining));

            if (remaining == 0)
            {
                LOG_MESSAGE(std::string(mName) + " All dependencies resolved, " + mName + " is ready");
                execute();
            }
        }

        void job::set_on_ready_callback(const std::function<void()> &pCallback)
        {
            LOG_MESSAGE(std::string(mName) + " Setting on_ready callback for job");
            if (mOnReady)
            {
                LOG_MESSAGE(std::string(mName) + " on_ready callback is already set, overwriting");
            }
            mOnReady = pCallback;
        }
    } // namespace jobs

} // namespace cacau