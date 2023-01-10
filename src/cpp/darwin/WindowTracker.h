#pragma once

#include <CoreFoundation/CoreFoundation.h>

#include <mutex>
#include <optional>
#include <thread>
#include <vector>

struct WindowData {
  uint32_t id;
  uint32_t parentPid;
  std::optional<std::string> windowName;
  std::optional<std::string> parentName;
  CGRect position;
};

// Responsible for tracking windows.
class WindowTracker {
  WindowTracker() : runWorker_(true) { worker_ = std::thread(&WindowTracker::UpdateWindows, this); }

 public:
  // Make this object a singleton.
  WindowTracker(const WindowTracker &) = delete;
  WindowTracker &operator=(const WindowTracker &) = delete;
  WindowTracker(WindowTracker &&) = delete;
  WindowTracker &operator=(WindowTracker &&) = delete;
  static auto &instance() {
    static WindowTracker _val;
    return _val;
  }

  ~WindowTracker() {
    runWorker_.store(false);
    if (worker_.joinable()) {
      worker_.join();
    }
  }
  void setOnStatus(std::function<void(bool)> onStatus) {
    std::unique_lock<std::mutex> lock_;
    statusIndicatorCbs_ = onStatus;
  }
  void setOnWindowPosition(std::function<void()> onWindowReset,
                           std::function<void(const CGRect &)> onWindowMove) {
    std::unique_lock<std::mutex> lock_;
    windowPositionCb_ = std::make_pair(onWindowReset, onWindowMove);
  }

  void stopTrackingWindow() {
    std::unique_lock<std::mutex> lock_;
    targetWindowId_.reset();
  }
  void trackWindow(uint32_t windowId) {
    std::unique_lock<std::mutex> lock_;
    targetWindowId_ = windowId;
  }

 private:
  void OnStatusIndicator(bool statusIndicatorActive) {
    std::unique_lock<std::mutex> lock_;
    if (statusIndicatorCbs_) statusIndicatorCbs_(statusIndicatorActive);
  }
  void OnTargetWindow(const WindowData *w) {
    std::unique_lock<std::mutex> lock_;
    if (w) {
      // On position callback.
      if (windowPositionCb_.second) windowPositionCb_.second(w->position);
    } else {
      // Reset callback.
      if (windowPositionCb_.first) windowPositionCb_.first();
    }
  }

  uint32_t GetTargetWindow() {
    std::unique_lock<std::mutex> lock_;
    return targetWindowId_.value_or(UINT32_MAX);
  }

  void UpdateWindows();

  std::atomic<bool> runWorker_;
  std::thread worker_;
  std::mutex lock_;
  std::optional<uint32_t> targetWindowId_;
  std::pair<std::function<void()>, std::function<void(const CGRect &)>> windowPositionCb_;
  std::function<void(bool)> statusIndicatorCbs_;
};
