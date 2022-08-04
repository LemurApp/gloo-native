#pragma once

#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <string>
#endif

#include <spdlog/spdlog.h>

#include <atomic>

#include "IDeviceManager.h"

namespace Gloo::Internal::MicDetector {

#ifdef __APPLE__
using AudioDeviceId = AudioObjectID;
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
using AudioDeviceId = std::string;
#endif

// Interface representing a device (either Mic or Speaker).
template <typename T>
class IDevice : public ITrackable {
 public:
  IDevice(AudioDeviceId id, IDeviceManager* manager)
      : _deviceId(id), _manager(manager) {
    spdlog::debug("Created device {}", _deviceId);
  }
  virtual ~IDevice() {
    spdlog::debug("Destroyed device {}", _deviceId);
    if constexpr (std::is_same_v<T, int>) {
      refreshState(0);
    } else if constexpr (std::is_same_v<T, bool>) {
      refreshState(false);
    }
  }

  // T getState() final const { return _deviceValue.load(); }

  void refreshState() {
    const T value = getStateFromDevice();
    refreshState(value, false);
  }

  void refreshState(T value, bool initialCall = false) {
    static_assert(std::is_same_v<T, int> || std::is_same_v<T, bool>,
                  "Only ints or bools allowed");
    const T prev = _deviceValue.exchange(value);
    spdlog::debug("REFRESHING DEVICE STATE: {} ({}->{}) [{}]", _deviceId, prev,
                  value, initialCall);
    if (initialCall || prev != value) {
      if constexpr (std::is_same_v<T, int>) {
        _manager->setVolume(value, initialCall);
      } else if constexpr (std::is_same_v<T, bool>) {
        _manager->setMicActivity(value, initialCall);
      }
    }
  }

 protected:
  virtual void startTrackingDeviceImpl() = 0;
  virtual void stopTrackingDeviceImpl() = 0;
  virtual T getStateFromDevice() const = 0;

  void stopTrackingImpl() final {
    spdlog::debug("Stopping device tracking {}", _deviceId);
    stopTrackingDeviceImpl();
  }
  void startTrackingImpl() final {
    spdlog::debug("Starting device tracking {}", _deviceId);
    refreshState(getStateFromDevice(), true);
    startTrackingDeviceImpl();
  }

  const AudioDeviceId _deviceId;

 private:
  IDeviceManager* const _manager;
  std::atomic<T> _deviceValue = 0;
};

using IMicrophoneDevice = IDevice<bool>;
using ISpeakerDevice = IDevice<int>;

}  // namespace Gloo::Internal::MicDetector
