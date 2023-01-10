#pragma once

#ifdef __APPLE__

#include <memory>
#include <unordered_set>

#include "../mic_detector/IDevice.h"
#include "../screen_tracker/WindowTracker.h"
#include "OSXHelpers.h"

namespace Gloo::Internal::MicDetector {
namespace Darwin {

class AirpodsMicrophoneDevice final : public IMicrophoneDevice {
 public:
  AirpodsMicrophoneDevice(AudioObjectID inObjectID, IDeviceManager* manager)
      : IMicrophoneDevice(inObjectID, manager) {}

  ~AirpodsMicrophoneDevice() {}

 private:
  bool getStateFromDevice() const { return state_.load(); }

  void startTrackingDeviceImpl() {
    // No-ops since WindowTracker is always running.
    WindowTracker::instance().setOnStatus([this](bool status) {
      const bool prev = state_.exchange(status);
      if (prev != status) {
        this->refreshState();
      }
    });
  }
  void stopTrackingDeviceImpl() {
    // No-ops since WindowTracker is always running.
    WindowTracker::instance().setOnStatus(nullptr);
  }

  std::atomic<bool> state_;
};

class MicrophoneDevice final : public IMicrophoneDevice {
 public:
  static OSStatus OnListener(AudioObjectID inObjectID, UInt32 inNumberAddresses,
                             const AudioObjectPropertyAddress* inAddresses,
                             void* raw) {
    const auto obj = static_cast<MicrophoneDevice*>(raw);
    switch (inAddresses->mSelector) {
      case kAudioDevicePropertyDeviceIsRunningSomewhere:
      case kAudioDevicePropertyDeviceIsRunning:
        obj->refreshState();
    }

    return 0;
  }

  MicrophoneDevice(AudioDeviceId deviceId, IDeviceManager* manager)
      : IMicrophoneDevice(deviceId, manager) {}

  ~MicrophoneDevice() { stopTracking(); }

 private:
  bool getStateFromDevice() const {
    return OSX_IsDeviceInUseByOthers(_deviceId);
  }

  void startTrackingDeviceImpl() {
    OSX_Subscribe(_deviceId, kAudioDevicePropertyDeviceIsRunningSomewhere,
                  this);
  }
  void stopTrackingDeviceImpl() {
    OSX_Unsubscribe(_deviceId, kAudioDevicePropertyDeviceIsRunningSomewhere,
                    this);
  }
};
}  // namespace Darwin

}  // namespace Gloo::Internal::MicDetector
#endif
