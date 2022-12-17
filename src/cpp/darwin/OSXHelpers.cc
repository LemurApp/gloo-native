#ifdef __APPLE__

#include "OSXHelpers.h"

#include <unordered_map>

#include <CoreAudio/CoreAudio.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <CoreGraphics/CoreGraphics.h>

namespace Gloo::Internal::MicDetector::Darwin {

void printAudioDeviceName(AudioObjectID deviceId) {
  // Get the default input device
  AudioObjectPropertyAddress propertyAddress = {
    kAudioHardwarePropertyDefaultInputDevice,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain
  };
  AudioDeviceID deviceID;
  UInt32 size = sizeof(deviceID);
  OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, nullptr, &size, &deviceID);
  if (status != noErr) {
    std::cerr << "Failed to get default audio device: " << status << std::endl;
    return;
  }

  // Get the device name
  CFStringRef deviceName;
  size = sizeof(deviceName);
  propertyAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
  status = AudioObjectGetPropertyData(deviceID, &propertyAddress, 0, nullptr, &size, &deviceName);
  if (status != noErr) {
    std::cerr << "Failed to get audio device name: " << status << std::endl;
    return;
  }

  // Convert the device name to a C string and print it
  char buffer[256];
  CFStringGetCString(deviceName, buffer, sizeof(buffer), kCFStringEncodingUTF8);
  std::cout << "Audio device name: " << buffer << std::endl;

  // Check if the device is being used
  UInt32 isBeingUsed;
  size = sizeof(isBeingUsed);
  propertyAddress.mSelector = kAudioDevicePropertyDeviceIsRunningSomewhere;
  status = AudioObjectGetPropertyData(deviceID, &propertyAddress, 0, nullptr, &size, &isBeingUsed);
  if (status != noErr) {
    std::cerr << "Failed to get audio device usage: " << status << std::endl;
    return;
  }

  std::cout << "Device is being used: " << (isBeingUsed ? "Yes" : "No") << std::endl;

  // Get the transport type of the default input device
      UInt32 dataSize = sizeof(deviceID);
    UInt32 transportType = 0;
    dataSize = sizeof(transportType);
    AudioObjectPropertyAddress transportTypeAddress = {
        kAudioDevicePropertyTransportType,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    OSStatus result = AudioObjectGetPropertyData(deviceID, &transportTypeAddress, 0, NULL, &dataSize, &transportType);
    if (result != kAudioHardwareNoError)
    {
        std::cerr << "Error getting transport type: " << result << std::endl;
        return;
    }

    // Print the transport type
    std::cout << "Transport type: " << transportType << std::endl;
    std::cout << "Transport type: " << kAudioDeviceTransportTypeBluetooth << std::endl;


  //  // Check if the device for other stuff
  // UInt32 placeholder;
  // size = sizeof(placeholder);
  // propertyAddress.mSelector = kAudioDevicePropertyProcessMute;
  // status = AudioObjectGetPropertyData(deviceID, &propertyAddress, 0, nullptr, &size, &placeholder);
  // if (status != noErr) {
  //   std::cerr << "Failed to get status: " << status << std::endl;
  //   return;
  // }

  // std::cout << "Audio device property " << (placeholder ? "Yes" : "No") << std::endl;

  // Release the device name
  CFRelease(deviceName);

   // Create an array of window dictionaries for the menu bar items
  CFArrayRef windowArray = CGWindowListCreate(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
  CFArrayRef windowDescriptionArray = CGWindowListCreateDescriptionFromArray(windowArray);
  CFRelease(windowArray);

  // Iterate over the window dictionaries
  for (CFIndex i = 0; i < CFArrayGetCount(windowDescriptionArray); ++i) {
    // Get the window dictionary for the current menu bar item
    CFDictionaryRef windowDictionary = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowDescriptionArray, i));

    // Get the window name
    CFStringRef windowName;
    CFDictionaryGetValueIfPresent(windowDictionary, kCGWindowName, reinterpret_cast<const void**>(&windowName));

    // Convert the window name to a C string and print it
    char buffer[256];
    CFStringGetCString(windowName, buffer, sizeof(buffer), kCFStringEncodingUTF8);
    std::cout << "Menu bar item " << (i + 1) << ": " << buffer << std::endl;
  }

  CFRelease(windowDescriptionArray);


      // Get the active window
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (windowList == NULL)
    {
        std::cerr << "Error getting window list" << std::endl;
        return;
    }

    CFIndex count = CFArrayGetCount(windowList);
    for (CFIndex i = 0; i < count; i++)
    {
        CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);
        CFNumberRef windowNumber = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowNumber);
        CFBooleanRef windowOnScreen = (CFBooleanRef)CFDictionaryGetValue(window, kCGWindowIsOnscreen);
        CFBooleanRef windowLayer = (CFBooleanRef)CFDictionaryGetValue(window, kCGWindowLayer);

        if (CFBooleanGetValue(windowOnScreen) && CFBooleanGetValue(windowLayer))
        {
            // The active window has the highest layer value
            int windowID;
            CFNumberGetValue(windowNumber, kCFNumberIntType, &windowID);
            std::cout << "Active window ID: " << windowID << std::endl;

            // Get the name of the active window
            CFStringRef windowName = (CFStringRef)CFDictionaryGetValue(window, kCGWindowName);
            if (windowName != NULL)
            {
                std::cout << "Active window name: " << CFStringGetCStringPtr(windowName, kCFStringEncodingUTF8) << std::endl;
            }
            else
            {
                std::cout << "Active window has no name" << std::endl;
            }
        }
    }

    CFRelease(windowList);
}


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

}  // namespace Gloo::Internal::MicDetector::Darwin

#endif