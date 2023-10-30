#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

using namespace std;

template <typename T>
struct ConcurrentQueue {
 private:
  deque<T> queue{};
  mutex popWaitCondVarMtx;
  condition_variable popWaitCondVar;

 public:
  ConcurrentQueue() {}
  ConcurrentQueue(const ConcurrentQueue&) = delete;
  ~ConcurrentQueue() {}

  void push(T e) {
    lock_guard<mutex> lock(popWaitCondVarMtx);
    queue.push_back(e);
    popWaitCondVar.notify_one();
  }

  T pop() {
    unique_lock<mutex> lock(popWaitCondVarMtx);
    popWaitCondVar.wait(lock, [this] { return !queue.empty(); });
    T e = queue.front();
    queue.pop_front();
    return e;
  }
};
