#pragma once

#include <spdlog/spdlog.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "IDevice.h"
#include "IDeviceManager.h"

#ifdef __APPLE__
#include "../darwin/MicrophoneDevice.h"
#endif

namespace Gloo::Internal::MicDetector {

class DeviceManager : public IDeviceManager {
 public:
  DeviceManager(IDeviceManager::OnMicChangeCallback cb0,
                IDeviceManager::OnVolumeChangeCallback cb1)
      : IDeviceManager(cb0, cb1) {}
  virtual ~DeviceManager() {}

 protected:
  void startTrackingImpl() final {
    spdlog::debug("Starting tracking for all devicess");
    IDeviceManager::startTrackingImpl();

    // First get all devices.
    refreshDeviceList(/*maybeInitializeDevice=*/false);

    // Add listeners to device level changes.
    startDeviceCallbacks();

    // Start tracking every device.
    auto devices = deviceList();
    for (auto& dev : devices) {
      dev->startTracking();
    }
  }

  void stopTrackingImpl() final {
    spdlog::debug("Stopping tracking for all devices");
    // Stop listeners to device level changes.
    stopDeviceCallbacks();

    // Stop tracking every device.
    auto devices = deviceList();
    for (auto& dev : devices) {
      dev->stopTracking();
    }
  }

  std::vector<std::shared_ptr<IMicrophoneDevice>> deviceList() {
    std::unique_lock<std::mutex> lk_(_m);
    std::vector<std::shared_ptr<IMicrophoneDevice>> devices;
    for (auto& [_, dev] : _mics) {
      devices.push_back(dev);
    }
    return devices;
  }

  // OS specific calls.
  virtual void startDeviceCallbacks() = 0;
  virtual void stopDeviceCallbacks() = 0;
  virtual void refreshDeviceList(bool maybeInitializeDevice) = 0;

  std::mutex _m;
  std::unordered_map<AudioDeviceId, std::shared_ptr<IMicrophoneDevice>> _mics;
  std::unordered_map<AudioDeviceId, std::shared_ptr<ISpeakerDevice>> _speakers;
};

std::shared_ptr<DeviceManager> MakeDeviceManager(
    IDeviceManager::OnMicChangeCallback cb0,
    IDeviceManager::OnVolumeChangeCallback cb1);
};  // namespace Gloo::Internal::MicDetector
