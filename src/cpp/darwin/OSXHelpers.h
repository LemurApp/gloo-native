#pragma once

#ifdef __APPLE__

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 120000
const AudioObjectPropertyScope kAudioObjectPropertyElementMain =
    kAudioObjectPropertyElementMaster;
#endif
#endif

namespace Gloo::Internal::MicDetector::Darwin {

uint32_t OSX_SafeAudioObjectGetPropertySize(
    AudioObjectID deviceId, const AudioObjectPropertyAddress &props);

std::string OSX_FromCfString(CFStringRef input);

template <typename T>
class DataBuffer {
 public:
  DataBuffer(uint32_t size) : buffer_(size), count_(size / sizeof(T)) {}

  void forEach(std::function<void(T)> cb) {
    const T *content = cast_data();
    for (uint32_t i = 0; i < count_; ++i) {
      cb(content[i]);
    }
  }

  const T *cast_data() const { return reinterpret_cast<const T *>(data()); }
  T *cast_data() { return reinterpret_cast<T *>(data()); }

  const void *data() const { return buffer_.data(); }
  void *data() { return buffer_.data(); }

 private:
  std::vector<uint8_t> buffer_;
  const uint32_t count_;
};

template <typename T>
DataBuffer<T> SafeAudioObjectGetPropertyValue(
    AudioObjectID deviceId, const AudioObjectPropertyAddress &props,
    uint32_t size = 0) {
  if (size == 0) {
    size = OSX_SafeAudioObjectGetPropertySize(deviceId, props);
  }

  DataBuffer<T> buffer(size);
  const auto err = AudioObjectGetPropertyData(deviceId, &props, 0, nullptr,
                                              &size, buffer.data());
  if (err) {
    throw std::system_error(EDOM, std::generic_category(), "hello world");
  }
  return buffer;
}

DataBuffer<AudioObjectID> OSX_GetAudioDeviceHandles();

std::string OSX_GetAudioDeviceUids(AudioObjectID deviceId);

bool OSX_IsInputDevice(AudioObjectID deviceId);

bool OSX_IsDeviceInUseBySelf(AudioObjectID deviceId);

bool OSX_IsDeviceInUseByOthers(AudioObjectID deviceId);

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

}  // namespace Gloo::Internal::MicDetector::Darwin

#endif