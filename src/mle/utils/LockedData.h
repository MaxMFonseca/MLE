#pragma once

#include <mutex>

namespace mle {
template <typename T>
class LockedData {
  public:
    LockedData(T& data, std::mutex& mtx) :
        lock_(mtx),
        data_(data) {}

    T& get() { return data_; }
    const T& get() const { return data_; }

    T* operator->() { return &data_; }
    const T* operator->() const { return &data_; }

    T& operator*() { return data_; }
    const T& operator*() const { return data_; }

  private:
    std::unique_lock<std::mutex> lock_;
    T& data_;
};

}  // namespace mle
