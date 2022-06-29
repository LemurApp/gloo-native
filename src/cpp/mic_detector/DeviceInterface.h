#pragma once

#include <string>

namespace Gloo::Internal::MicDetector {

struct InputDevice {
    uint32_t deviceId;
    std::string name;
};

struct DeviceStates {
    bool isDeviceInUse;
};


// Returns a list of devices that are online.
std::vector<InputDevice> GetInputDevices();

// Updates the vector in place, to the deviceInUse field.
std::vector<DeviceStates> GetDeviceActiveState(std::vector<InputDevice>& devices);

};
