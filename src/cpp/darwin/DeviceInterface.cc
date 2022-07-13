#include "../mic_detector/DeviceInterface.h"
#include "DeviceApi.h"
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 120000
const AudioObjectPropertyScope kAudioObjectPropertyElementMain =
    kAudioObjectPropertyElementMaster;
#endif
#endif

namespace Gloo::Internal::MicDetector {
using Darwin::DataBuffer;
using Darwin::SafeAudioObjectGetPropertyValue;

std::string OSX_FromCfString(CFStringRef input) {
  if (input == nullptr)
    return std::string();
  CFIndex string_length = CFStringGetLength(input);
  CFIndex string_max_length =
      CFStringGetMaximumSizeForEncoding(string_length, kCFStringEncodingASCII);
  std::string buffer(string_max_length + 1, '\0');
  if (!CFStringGetCString(input, buffer.data(), string_max_length + 1,
                          kCFStringEncodingASCII)) {
    throw std::invalid_argument("Unable to convert string");
  }
  return buffer;
}

DataBuffer<AudioObjectID> OSX_GetAudioDeviceHandles() {
  AudioObjectPropertyAddress props = {kAudioHardwarePropertyDevices,
                                      kAudioObjectPropertyScopeGlobal,
                                      kAudioObjectPropertyElementMain};
  const auto devices = SafeAudioObjectGetPropertyValue<AudioObjectID>(
      kAudioObjectSystemObject, props);
  return devices;
}

std::string OSX_GetAudioDeviceUids(AudioObjectID deviceId) {
  static std::unordered_map<AudioObjectID, std::string> cache_;
  if (cache_.count(deviceId) == 0) {
    AudioObjectPropertyAddress props = {kAudioDevicePropertyDeviceUID,
                                        kAudioObjectPropertyScopeGlobal,
                                        kAudioObjectPropertyElementMain};
    const auto raw_uid =
        SafeAudioObjectGetPropertyValue<CFStringRef>(deviceId, props);
    std::string uid(OSX_FromCfString(*raw_uid.cast_data()));
    cache_.insert_or_assign(deviceId, uid);
  }
  return cache_.at(deviceId);
}

bool OSX_IsInputDevice(AudioObjectID deviceId) {
  static std::unordered_map<AudioObjectID, bool> cache_;
  if (cache_.count(deviceId) == 0) {
    AudioObjectPropertyAddress props = {kAudioDevicePropertyStreamConfiguration,
                                        kAudioObjectPropertyScopeInput,
                                        kAudioObjectPropertyElementWildcard};
    const auto ret =
        SafeAudioObjectGetPropertyValue<AudioBufferList>(deviceId, props)
            .cast_data()
            ->mNumberBuffers > 0;
    cache_.insert_or_assign(deviceId, ret);
  }

  return cache_.at(deviceId);
}

bool OSX_IsDeviceInUseBySelf(AudioObjectID deviceId) {
  AudioObjectPropertyAddress props = {kAudioDevicePropertyDeviceIsRunning,
                                      kAudioObjectPropertyScopeGlobal,
                                      kAudioObjectPropertyElementMain};
  const auto ret =
      *(SafeAudioObjectGetPropertyValue<uint32_t>(deviceId, props).cast_data());
  return ret != 0;
}

bool OSX_IsDeviceInUseByOthers(AudioObjectID deviceId) {
  AudioObjectPropertyAddress props = {
      kAudioDevicePropertyDeviceIsRunningSomewhere,
      kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
  const auto ret =
      *(SafeAudioObjectGetPropertyValue<uint32_t>(deviceId, props).cast_data());
  return ret != 0;
}

template <typename T>
void OSX_Subscribe(AudioDeviceID device, AudioObjectPropertySelector prop,
                   T *data) {
  AudioObjectPropertyAddress props = {prop, kAudioObjectPropertyScopeGlobal,
                                      kAudioObjectPropertyElementMain};
  AudioObjectAddPropertyListener(device, &props, &T::OnListener,
                                 static_cast<void *>(data));
}

template <typename T>
void OSX_Unsubscribe(AudioDeviceID device, AudioObjectPropertySelector prop,
                     T *data) {
  AudioObjectPropertyAddress props = {prop, kAudioObjectPropertyScopeGlobal,
                                      kAudioObjectPropertyElementMain};
  AudioObjectRemovePropertyListener(device, &props, &T::OnListener,
                                    static_cast<void *>(data));
}

class OSXMicrophoneDevice {
public:
  OSXMicrophoneDevice(AudioObjectID id, DeviceManager *owner)
      : owner_(owner), inUse_(false), deviceId(id) {
    UpdateInUse(false);
  }

  ~OSXMicrophoneDevice() {
    std::cout << "Destroying object: " << deviceId << std::endl;
    stop();
  }

  void start() {
    const bool prev = monitored_.exchange(true);
    if (prev)
      return;
    OSX_Subscribe(deviceId, kAudioDevicePropertyDeviceIsRunningSomewhere, this);
  }
  void stop() {
    const bool prev = monitored_.exchange(false);
    if (prev) {
      OSX_Unsubscribe(deviceId, kAudioDevicePropertyDeviceIsRunningSomewhere,
                      this);
    }
  }

  bool IsActive() const { return inUse_.load(); }

  AudioObjectID id() const { return deviceId; }

  static OSStatus OnListener(AudioObjectID inObjectID, UInt32 inNumberAddresses,
                             const AudioObjectPropertyAddress *inAddresses,
                             void *raw) {
    const auto obj = static_cast<OSXMicrophoneDevice *>(raw);
    switch (inAddresses->mSelector) {
    case kAudioDevicePropertyDeviceIsRunningSomewhere:
    case kAudioDevicePropertyDeviceIsRunning:
      obj->UpdateInUse(true);
    }

    return 0;
  }

private:
  void UpdateInUse(bool withRefresh) {
    const auto others = OSX_IsDeviceInUseByOthers(deviceId);
    const auto self = OSX_IsDeviceInUseBySelf(deviceId);
    const auto inUse = self ? false : others;
    inUse_.store(inUse);
    if (withRefresh)
      owner_->RefreshDeviceState();
  }

  DeviceManager *owner_;

  std::atomic<bool> inUse_;
  std::atomic<bool> monitored_{false};
  const AudioObjectID deviceId;
};

class OSXDeviceManager final : public DeviceManager {
public:
  OSXDeviceManager(DeviceManager::Callback cb) : DeviceManager(cb) {
    UpdateDeviceList();
  }

  static OSStatus OnListener(AudioObjectID inObjectID, UInt32 inNumberAddresses,
                             const AudioObjectPropertyAddress *inAddresses,
                             void *raw) {
    const auto obj = static_cast<OSXDeviceManager *>(raw);
    switch (inAddresses->mSelector) {
    case kAudioHardwarePropertyDevices:
      obj->UpdateDeviceList();
    }

    return 0;
  }

private:
  void startImpl() {
    OSX_Subscribe(kAudioObjectSystemObject, kAudioHardwarePropertyDevices,
                  this);
    {
      std::unique_lock<std::mutex> lk_(m_);
      for (auto &d : devices) {
        d->start();
      }
    }
  }

  void stopImpl() {
    OSX_Unsubscribe(kAudioObjectSystemObject, kAudioHardwarePropertyDevices,
                    this);
    {
      std::unique_lock<std::mutex> lk_(m_);
      for (auto &d : devices) {
        d->stop();
      }
    }
  }

  void UpdateDeviceList() {
    std::unordered_set<AudioObjectID> input_devices;
    auto device_list = OSX_GetAudioDeviceHandles();
    device_list.forEach([&](AudioObjectID d) {
      if (OSX_IsInputDevice(d)) {
        input_devices.insert(d);
      }
    });
    {
      std::unique_lock<std::mutex> lk_(m_);
      devices.erase(std::remove_if(devices.begin(), devices.end(),
                                   [&](const auto &item) {
                                     if (input_devices.count(item->id())) {
                                       input_devices.erase(item->id());
                                       return false;
                                     }
                                     return true;
                                   }),
                    devices.end());
      for (auto &d : input_devices) {
        devices.push_back(std::make_shared<OSXMicrophoneDevice>(d, this));
      }
    }
    RefreshDeviceState();
  }

  bool IsActive() {
    std::unique_lock<std::mutex> lk_(m_);
    for (const auto &d : devices) {
      if (d->IsActive()) {
        return true;
      }
    }
    return false;
  }

  std::mutex m_;
  std::vector<std::shared_ptr<OSXMicrophoneDevice>> devices;
};

std::shared_ptr<DeviceManager> MakeDeviceManager(DeviceManager::Callback cb) {
  return std::shared_ptr<DeviceManager>(new OSXDeviceManager(cb));
}
} // namespace Gloo::Internal::MicDetector
