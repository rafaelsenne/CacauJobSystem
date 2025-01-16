#include "job.h"

namespace cacau 
{
    namespace jobs
    {

        void job::execute()
        {
            LOG_MESSAGE(std::string(m_name) + " Executing job " + std::string(m_name));
            if (m_function)
            {
                m_function();
            }
            mIsFinished = true;
            {
                std::lock_guard<std::mutex> lock(m_dependants_mutex);
                auto dependants = m_dependants;
                for (auto *dependant : dependants)
                {
                    if (dependant == nullptr)
                    {
                        LOG_MESSAGE(std::string(m_name) + " Dependant job is null, skipping");
                        continue;
                    }
                    if (!dependant->is_ready())
                    {
                        dependant->resolve_dependency(m_name);
                    }
                }
            }

            LOG_MESSAGE(std::string(m_name) + " Exiting execute");
        }

        bool job::add_dependant(job *dependant)
        {
            if (is_finished())
            {
                return false;
            }

            {
                std::lock_guard<std::mutex> lock(m_dependants_mutex);
                LOG_MESSAGE(std::string(m_name) + " Attempting to lock m_dependants_mutex");

                LOG_MESSAGE(std::string(m_name) + " Adding dependant " + std::string(dependant->m_name));
                m_dependants.push_back(dependant);
                LOG_MESSAGE(std::string(m_name) + " added dependant " + std::string(dependant->m_name));
                dependant->add_dependency(this);
            }

            LOG_MESSAGE(std::string(m_name) + " Exiting add_dependant");
            return true;
        }

        void job::add_dependency(job *dependency)
        {
            m_remaining_dependencies.fetch_add(1, std::memory_order_relaxed);
            LOG_MESSAGE(std::string(m_name) + " Dependency added, remaining: " +
                        std::to_string(m_remaining_dependencies.load()));
        }

        void job::resolve_dependency(const char *caller)
        {
            int remaining = m_remaining_dependencies.fetch_sub(1, std::memory_order_acq_rel) - 1;
            LOG_MESSAGE(std::string(m_name) + " Resolving dependency [" + caller +
                        "], remaining: " + std::to_string(remaining));

            if (remaining == 0)
            {
                LOG_MESSAGE(std::string(m_name) + " All dependencies resolved, " + m_name + " is ready");
                execute();
            }
        }

        void job::set_on_ready_callback(const std::function<void()> &callback)
        {
            LOG_MESSAGE(std::string(m_name) + " Setting on_ready callback for job");
            if (m_on_ready)
            {
                LOG_MESSAGE(std::string(m_name) + " on_ready callback is already set, overwriting");
            }
            m_on_ready = callback;
        }
    } // namespace jobs

} // namespace cacau