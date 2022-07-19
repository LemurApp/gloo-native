#pragma once

#include "ITrackable.h"

namespace Gloo::Internal::MicDetector {

class IDeviceManager : public ITrackable {
 public:
  enum class MicActivity {
    kUnknown,
    kOff,
    kOn,
  };

  enum class VolumeActivity {
    kMuted,
    kUnmuted,
  };

  using OnMicChangeCallback = std::function<void(MicActivity)>;
  using OnVolumeChangeCallback = std::function<void(VolumeActivity)>;

  IDeviceManager(OnMicChangeCallback onMicChange,
                 OnVolumeChangeCallback onVolumeChange)
      : _onMicChange(onMicChange), _onVolumeChange(onVolumeChange) {}
  virtual ~IDeviceManager() {}

  void setVolume(int volume, bool initialCall) {
    auto prev = _volume.fetch_add(volume);
    if (initialCall || prev != volume) {
      _onVolumeChange(volume == 0 ? VolumeActivity::kMuted
                                  : VolumeActivity::kUnmuted);
    }
  }

  void setMicActivity(bool active, bool initialCall) {
    auto increment = active ? 1 : (initialCall ? 0 : -1);
    auto prev = _activeMicCount.fetch_add(increment);
    auto update =
        (prev + increment == 0) ? MicActivity::kOff : MicActivity::kOn;
    auto prevActive = _anyMicActive.exchange(update);
    if (prevActive != update) {
      _onMicChange(update);
    }
  }

 protected:
  virtual void startTrackingImpl() {
    _activeMicCount.store(0);
    _anyMicActive.store(MicActivity::kUnknown);
    _volume.store(-1);
  }

 private:
  std::atomic<int> _activeMicCount;
  std::atomic<MicActivity> _anyMicActive;
  std::atomic<int> _volume;

  const OnMicChangeCallback _onMicChange;
  const OnVolumeChangeCallback _onVolumeChange;
};

}  // namespace Gloo::Internal::MicDetector
