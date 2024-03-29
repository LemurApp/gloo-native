#pragma once

#include <napi.h>

#include <condition_variable>
#include <memory>
#include <thread>
#include <unordered_map>

#include "../napi-thread-safe-callback.h"
#include "DeviceManager.h"

namespace Gloo::Internal::MicDetector {
enum class MicrophoneState : int {
  ON,
  OFF,
};

class MicrophoneDetector {
  typedef std::shared_ptr<ThreadSafeCallback> MicStatusCallback;

 public:
  // Make this object a singleton.
  MicrophoneDetector(const MicrophoneDetector &) = delete;
  MicrophoneDetector &operator=(const MicrophoneDetector &) = delete;
  MicrophoneDetector(MicrophoneDetector &&) = delete;
  MicrophoneDetector &operator=(MicrophoneDetector &&) = delete;
  static auto &instance() {
    static MicrophoneDetector _val;
    return _val;
  }

  // Starts an internal thread which monitors for microphone usage.
  void resume();
  // Pauses an internal thread which monitors for microphone usage.
  void pause();

  // Configures the function which is called whenever the function enables.
  int registerCallback(MicStatusCallback callback);
  void unregisterCallback(int callbackId);

 private:
  MicrophoneDetector() {
    mgr_ = MakeDeviceManager(
        [&](IDeviceManager::MicActivity state) { this->stateChanged(state); },
        [&](IDeviceManager::VolumeActivity volume) {
          std::cout << "Volume changed to: " << static_cast<int>(volume)
                    << std::endl;
        });
  }

  // Objects for thread safety.
  mutable std::mutex
      m_;  // Used for safely accessing all members: callbacks_, thread_active_
  std::unordered_map<int, MicStatusCallback> callbacks_;

  // Used to fire callbacks.
  void stateChanged(IDeviceManager::MicActivity state) const;

  std::shared_ptr<DeviceManager> mgr_;
};
}  // namespace Gloo::Internal::MicDetector
