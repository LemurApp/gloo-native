#ifdef __APPLE__

#include "OSXHelpers.h"

#include <unordered_map>

namespace Gloo::Internal::MicDetector::Darwin {

uint32_t OSX_SafeAudioObjectGetPropertySize(
    AudioObjectID deviceId, const AudioObjectPropertyAddress &props) {
  if (!AudioObjectHasProperty(deviceId, &props)) {
    throw std::invalid_argument("No such property on device");
  }

  uint32_t property_size;
  const auto err = AudioObjectGetPropertyDataSize(deviceId, &props, 0, nullptr,
                                                  &property_size);
  if (err) {
    throw std::system_error(EDOM, std::generic_category(), "hello world");
  }
  return property_size;
}

std::string OSX_FromCfString(CFStringRef input) {
  if (input == nullptr) return std::string();
  CFIndex string_length = CFStringGetLength(input);
  CFIndex string_max_length = CFStringGetMaximumSizeForEncoding(
      string_length, CFStringGetSystemEncoding());
  std::string buffer(string_max_length + 1, '\0');
  if (!CFStringGetCString(input, buffer.data(), string_max_length + 1,
                          CFStringGetSystemEncoding())) {
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

}  // namespace Gloo::Internal::MicDetector::Darwin

#endif