#pragma once

#include "dxvk_include.h"

#include <functional>
#include <future>
#include <queue>
#include <vector>

namespace dxvk {

class DxvkThreadPool {
public:
  DxvkThreadPool(ThreadPriority priority = ThreadPriority::Normal,
                 uint32_t numCompilerThreads = 0);
  ~DxvkThreadPool();

  template<class T, class... Args>
  void enqueue(T&& task, Args&&... args);

  uint32_t running() { return m_workerBusy.load(); }

private:
  uint32_t numWorkers;
  std::atomic<bool> m_stopThreads = { false };

  std::mutex m_workerLock;
  std::condition_variable m_workerCond;
  std::queue<std::function<void()>> m_workerQueue;
  std::atomic<uint32_t> m_workerBusy;
  std::vector<dxvk::thread> m_workerThreads;
};

template<class T, class... Args>
void DxvkThreadPool::enqueue(T&& task, Args&&... args) {
  std::lock_guard<std::mutex> lock(m_workerLock);

  if (m_stopThreads.load())
    throw std::runtime_error("DxvkThreadPool::enqeue: stopped");

  m_workerQueue.emplace(std::bind(std::forward<T>(task), std::forward<Args>(args)...));
  m_workerCond.notify_one();
}

}