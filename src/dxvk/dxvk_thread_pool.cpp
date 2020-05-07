#include "dxvk_thread_pool.h"

namespace dxvk {

DxvkThreadPool::DxvkThreadPool(dxvk::ThreadPriority priority,
                               uint32_t numCompilerThreads) {
  // Use half the available CPU cores for pipeline compilation
  uint32_t numCpuCores = dxvk::thread::hardware_concurrency();
  numWorkers = ((std::max(1u, numCpuCores) - 1) * 5) / 7;

  if (numWorkers < 1)
    numWorkers = 1;
  if (numWorkers > 32)
    numWorkers = 32;

  if (numCompilerThreads > 0)
    numWorkers = numCompilerThreads;

  Logger::info(str::format("DXVK: Using ", numWorkers, " compiler threads"));

  // Start the worker threads and the file writer
  m_workerBusy.store(numWorkers);

  for (uint32_t i = 0; i < numWorkers; i++) {
    m_workerThreads.emplace_back([this]() {
      while (!m_stopThreads.load()) {
        std::function<void()> task;

        {
          std::unique_lock<std::mutex> lock(m_workerLock);

          if (m_workerQueue.empty()) {
            m_workerBusy -= 1;
            m_workerCond.wait(lock, [this]() {
              return !m_workerQueue.empty() || m_stopThreads.load();
            });

            if (!m_workerQueue.empty())
              m_workerBusy += 1;
          }

          if (m_workerQueue.empty())
            break;

          task = std::move(m_workerQueue.front());
          m_workerQueue.pop();
        }

        task();

      }
    });
    m_workerThreads[i].set_priority(priority);
  }
}

DxvkThreadPool::~DxvkThreadPool() {
  {
    std::lock_guard<std::mutex> workerLock(m_workerLock);
    m_stopThreads.store(true);
    m_workerCond.notify_all();
  }

  for (auto &worker : m_workerThreads)
    worker.join();
}

} // namespace dxvk