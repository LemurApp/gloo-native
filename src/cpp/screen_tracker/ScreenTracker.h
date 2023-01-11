#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>

#include "../napi-thread-safe-callback.h"
#include "WindowTracker.h"

class ScreenTracker {
  typedef std::shared_ptr<ThreadSafeCallback> ScreenChangeCallback;

 public:
  // Make this object a singleton.
  ScreenTracker(const ScreenTracker &) = delete;
  ScreenTracker &operator=(const ScreenTracker &) = delete;
  ScreenTracker(ScreenTracker &&) = delete;
  ScreenTracker &operator=(ScreenTracker &&) = delete;
  static auto &instance() {
    static ScreenTracker _val;
    return _val;
  }

  int registerCallback(ScreenChangeCallback callback) {
    std::unique_lock<std::mutex> lk(m_);

    const int nextId = this->callbacks_.size();
    this->callbacks_[nextId] = callback;
    return nextId;
  }

  void unregisterCallback(int callbackId) {
    std::unique_lock<std::mutex> lk(m_);

    this->callbacks_.erase(callbackId);
  }

 private:
  ScreenTracker() : visible_(false) {
    WindowTracker::instance().setOnWindowPosition(
        [this]() {
          const bool prev = visible_.exchange(false);
          if (prev) {
            onChange(false, nullptr);
          }
        },
        [this](const WindowRect &rect) {
          const bool prevVis = visible_.exchange(true);
          const auto prev = rect_.exchange(rect);
          if (prevVis && memcmp(&prev, &rect, sizeof(rect)) == 0) {
            // No callback as nothing changed.
          } else {
            onChange(true, &rect);
          }
        });
  }

  void onChange(bool visibility, const WindowRect *rect);

  std::mutex m_;
  std::atomic<bool> visible_;
  std::atomic<WindowRect> rect_;
  std::unordered_map<int, ScreenChangeCallback> callbacks_;
};
