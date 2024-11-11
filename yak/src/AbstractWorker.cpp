#include "AbstractWorker.h"
#include "logging/Logging.h"

void AbstractWorker::set_queue(std::shared_ptr<SafeQueue<PollResult>> queue) { m_queue = queue; }

void AbstractWorker::run()
{
  Logging::INFO("Started", m_name);

  while (!m_sig_channel->m_shutdown_requested.load())
  {
    {
      std::unique_lock shutdown_lock(m_sig_channel->m_cv_mutex);
      m_sig_channel->m_cv.wait_for(shutdown_lock, std::chrono::milliseconds(10), [this]()
                                   { bool should_shutdown = m_sig_channel->m_shutdown_requested.load();
                                     return should_shutdown; });
    }

    step();
  }
  Logging::INFO("Shutting down", m_name);
  clean();
}
