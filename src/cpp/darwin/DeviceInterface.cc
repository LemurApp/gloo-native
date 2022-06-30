#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include "../mic_detector/DeviceInterface.h"
#include "DeviceApi.h"
#include <unordered_map>

#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 120000
const AudioObjectPropertyScope kAudioObjectPropertyElementMain = kAudioObjectPropertyElementMaster;
#endif
#endif

namespace Gloo::Internal::MicDetector
{
  using Darwin::DataBuffer;
  using Darwin::SafeAudioObjectGetPropertyValue;

  std::string OSX_FromCfString(CFStringRef input)
  {
    if (input == nullptr)
      return std::string();
    CFIndex string_length = CFStringGetLength(input);
    CFIndex string_max_length =
        CFStringGetMaximumSizeForEncoding(
            string_length,
            kCFStringEncodingASCII);
    std::string buffer(string_max_length + 1, '\0');
    if (!CFStringGetCString(input,
                            buffer.data(),
                            string_max_length + 1,
                            kCFStringEncodingASCII))
    {
      throw std::invalid_argument("Unable to convert string");
    }
    return buffer;
  }

  DataBuffer<AudioObjectID> OSX_GetAudioDeviceHandles()
  {
    AudioObjectPropertyAddress props = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain};
    const auto devices = SafeAudioObjectGetPropertyValue<AudioObjectID>(kAudioObjectSystemObject, props);
    return devices;
  }

  std::string OSX_GetAudioDeviceUids(AudioObjectID deviceId)
  {
    static std::unordered_map<AudioObjectID, std::string> cache_;
    if (cache_.count(deviceId) == 0)
    {
      AudioObjectPropertyAddress props = {
          kAudioDevicePropertyDeviceUID,
          kAudioObjectPropertyScopeGlobal,
          kAudioObjectPropertyElementMain};
      const auto raw_uid = SafeAudioObjectGetPropertyValue<CFStringRef>(deviceId, props);
      std::string uid(OSX_FromCfString(*raw_uid.cast_data()));
      cache_.insert_or_assign(deviceId, uid);
    }
    return cache_.at(deviceId);
  }

  bool OSX_IsInputDevice(AudioObjectID deviceId)
  {
    static std::unordered_map<AudioObjectID, bool> cache_;
    if (cache_.count(deviceId) == 0)
    {
      AudioObjectPropertyAddress props = {
          kAudioDevicePropertyStreamConfiguration,
          kAudioObjectPropertyScopeInput,
          kAudioObjectPropertyElementWildcard};
      const auto ret = SafeAudioObjectGetPropertyValue<AudioBufferList>(deviceId, props).cast_data()->mNumberBuffers > 0;
      cache_.insert_or_assign(deviceId, ret);
    }

    return cache_.at(deviceId);
  }

  bool OSX_IsDeviceInUseByOthers(AudioObjectID deviceId)
  {
    AudioObjectPropertyAddress props = {
        kAudioDevicePropertyDeviceIsRunningSomewhere,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain};
    const auto ret = *(SafeAudioObjectGetPropertyValue<uint32_t>(deviceId, props).cast_data());
    return ret != 0;
  }

  std::vector<InputDevice> GetInputDevices()
  {
    std::vector<InputDevice> result;
    auto devices = OSX_GetAudioDeviceHandles();
    devices.forEach([&](AudioObjectID d)
                    {
      if (OSX_IsInputDevice(d)) {
        const auto uid = OSX_GetAudioDeviceUids(d);
        result.emplace_back(InputDevice{d, uid});
      } });

    return result;
  }

  std::vector<DeviceStates> GetDeviceActiveState(std::vector<InputDevice> &devices)
  {
    std::vector<DeviceStates> states;
    for (const auto &d : devices)
    {
      states.emplace_back(DeviceStates{OSX_IsDeviceInUseByOthers(d.deviceId)});
    }
    return states;
  }

}
