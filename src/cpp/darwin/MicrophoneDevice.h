#pragma once

#ifdef __APPLE__

#include <memory>
#include <unordered_set>

#include "../mic_detector/IDevice.h"
#include "OSXHelpers.h"

namespace Gloo::Internal::MicDetector {
namespace Darwin {

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
