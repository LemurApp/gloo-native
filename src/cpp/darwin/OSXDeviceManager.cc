#ifdef __APPLE__

#include <algorithm>
#include <memory>
#include <unordered_set>

#include "../mic_detector/DeviceManager.h"
#include "MicrophoneDevice.h"
#include "OSXHelpers.h"

namespace Gloo::Internal::MicDetector {
namespace Darwin {

static const AudioObjectID kWindowManagerDeviceId = UINT32_MAX;

class OSXDeviceManager final : public DeviceManager {
 public:
  static OSStatus OnListener(AudioObjectID inObjectID, UInt32 inNumberAddresses,
                             const AudioObjectPropertyAddress* inAddresses,
                             void* raw) {
    const auto obj = static_cast<OSXDeviceManager*>(raw);
    switch (inAddresses->mSelector) {
      case kAudioHardwarePropertyDevices: {
        obj->refreshDeviceList(true);
      }
    }

    return 0;
  }

  OSXDeviceManager(IDeviceManager::OnMicChangeCallback cb0,
                   IDeviceManager::OnVolumeChangeCallback cb1)
      : DeviceManager(cb0, cb1) {
    _mics[kWindowManagerDeviceId] = std::shared_ptr<IMicrophoneDevice>(
        new AirpodsMicrophoneDevice(kWindowManagerDeviceId, this));
  }
  ~OSXDeviceManager() { stopTracking(); }

  void refreshDeviceList(bool maybeInitializeDevice) {
    // Get all input devices.
    std::unordered_set<AudioDeviceId> input_device_ids;
    auto device_list = OSX_GetAudioDeviceHandles();
    device_list.forEach([&](AudioDeviceId d) {
      if (OSX_IsInputDevice(d)) {
        input_device_ids.insert(d);
      }
    });

    std::vector<std::shared_ptr<IMicrophoneDevice>> devicesToStart;
    {
      std::unique_lock<std::mutex> lk_(_m);
      for (auto it = begin(_mics); it != end(_mics);) {
        const auto key = it->first;
        // Special key for airpods
        if (input_device_ids.count(key) == 0) {
          // Item is no longer present, remove it.
          if (key == kWindowManagerDeviceId) {
            // Special case for kWindowManagerDeviceId.
            ++it;
          } else {
            it = _mics.erase(it);
          }
        } else {
          // Already tracking this item, don't recreate it.
          input_device_ids.erase(key);
          ++it;
        }
      }
      bool withTrackingOn = isTracking();
      for (auto& id : input_device_ids) {
        auto device =
            std::shared_ptr<IMicrophoneDevice>(new MicrophoneDevice(id, this));
        _mics[id] = device;
        if (maybeInitializeDevice && withTrackingOn) {
          devicesToStart.push_back(device);
        }
      }
    }

    for (auto& dev : devicesToStart) {
      dev->startTracking();
    }
  }

  void startDeviceCallbacks() {
    OSX_Subscribe(kAudioObjectSystemObject, kAudioHardwarePropertyDevices,
                  this);
  }

  void stopDeviceCallbacks() {
    OSX_Unsubscribe(kAudioObjectSystemObject, kAudioHardwarePropertyDevices,
                    this);
  }
};
}  // namespace Darwin

std::shared_ptr<DeviceManager> MakeDeviceManager(
    IDeviceManager::OnMicChangeCallback cb0,
    IDeviceManager::OnVolumeChangeCallback cb1) {
  return std::shared_ptr<DeviceManager>(new Darwin::OSXDeviceManager(cb0, cb1));
}
}  // namespace Gloo::Internal::MicDetector
#endif
