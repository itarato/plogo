#pragma once

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>

using namespace std;

template <typename T>
struct ConcurrentDeque {
 private:
  deque<T> queue{};
  mutex popWaitCondVarMtx;
  condition_variable popWaitCondVar;

 public:
  ConcurrentDeque() {}
  ConcurrentDeque(const ConcurrentDeque &) = delete;
  ~ConcurrentDeque() {}

  void push_back(T e) {
    lock_guard<mutex> lock(popWaitCondVarMtx);
    queue.push_back(e);
    popWaitCondVar.notify_one();
  }

  T pop_front() {
    unique_lock<mutex> lock(popWaitCondVarMtx);
    popWaitCondVar.wait(lock, [this] { return !queue.empty(); });
    T e = queue.front();
    queue.pop_front();
    return e;
  }

  void popInto(vector<T> &other) {
    unique_lock<mutex> lock(popWaitCondVarMtx);

    if (queue.empty()) return;

    copy(queue.begin(), queue.end(), back_inserter(other));

    queue.clear();
  }

  void clear() {
    unique_lock<mutex> lock(popWaitCondVarMtx);
    queue.clear();
  }
};
