#pragma once

#include <atomic>

#include "IDeviceManager.h"

namespace Gloo::Internal::MicDetector {

class ITrackable {
 public:
  virtual ~ITrackable() {}

  void startTracking() {
    auto prev = _tracking.exchange(true);
    // Already tracking before this.
    if (prev) return;

    startTrackingImpl();
  }
  void stopTracking() {
    auto prev = _tracking.exchange(false);
    // Already not tracking before this.
    if (!prev) return;

    stopTrackingImpl();
  }
  bool isTracking() const { return _tracking.load(); }

 protected:
  virtual void startTrackingImpl() = 0;
  virtual void stopTrackingImpl() = 0;

 private:
  std::atomic<bool> _tracking;
};

}  // namespace Gloo::Internal::MicDetector
