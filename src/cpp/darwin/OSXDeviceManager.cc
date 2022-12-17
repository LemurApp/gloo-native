#ifdef __APPLE__

#include <algorithm>
#include <memory>
#include <unordered_set>

#include "../mic_detector/DeviceManager.h"
#include "MicrophoneDevice.h"
#include "OSXHelpers.h"
#include <ApplicationServices/ApplicationServices.h>
#include <iostream>

namespace Gloo::Internal::MicDetector {
namespace Darwin {


// Callback function that is called when the active window changes
void activeWindowChanged(AXObserverRef observer, AXUIElementRef element, CFStringRef notification, void *data) {
    std::cout << "Active window changed " << std::endl;
  // Get the new active window
  AXUIElementRef activeWindow;
  AXError error = AXUIElementCopyAttributeValue(element, kAXFocusedWindowAttribute, (CFTypeRef *)&activeWindow);
  if (error != kAXErrorSuccess) {
    // Failed to get the active window
    return;
  }

  // Get the name of the active window
  // CFStringRef windowName;
  // error = AXUIElementCopyAttributeValue(activeWindow, kAXTitleAttribute, (CFTypeRef *)&windowName);
  // if (error != kAXErrorSuccess) {
  //   // Failed to get the window name
  //   CFRelease(activeWindow);
  //   return;
  // }

  // Get the position of the active window
  // CGPoint windowPosition;
  // error = AXUIElementCopyAttributeValue(activeWindow, kAXPositionAttribute, (CFTypeRef *)&windowPosition);
  // if (error != kAXErrorSuccess) {
  //   // Failed to get the window position
  //  // CFRelease(windowName);
  //   CFRelease(activeWindow);
  //   return;
  // }

  // Print the name and position of the active window
  std::cout << "Active window changed " 
  // << CFStringGetCStringPtr(windowName, kCFStringEncodingUTF8) << " (" << windowPosition.x << ", " << windowPosition.y << ")"
   << std::endl;

  // Clean up
 // CFRelease(windowName);
  CFRelease(activeWindow);
}

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
  AXObserverRef observer;

  OSXDeviceManager(IDeviceManager::OnMicChangeCallback cb0,
                   IDeviceManager::OnVolumeChangeCallback cb1)
      : DeviceManager(cb0, cb1) {
            std::cout << "device manager " << std::endl;

  pid_t pid = getpid();
  // Create an accessibility observer
  AXError error = AXObserverCreate(pid, activeWindowChanged, &observer);
  if (error != kAXErrorSuccess) {
    // Failed to create the observer
   // return 1;
  }
  // Register the observer to receive notifications when the active window changes
  error = AXObserverAddNotification(observer, nullptr, kAXFocusedWindowChangedNotification, nullptr);
  if (error != kAXErrorSuccess) {
    // Failed to register the observer
    CFRelease(observer);
   // return 1;
  }
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
        if (input_device_ids.count(key) == 0) {
          // Item is no longer present, remove it.
          it = _mics.erase(it);
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
