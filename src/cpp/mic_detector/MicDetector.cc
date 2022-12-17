#include "MicDetector.h"

#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include <string>


#include "DeviceManager.h"
using namespace std::chrono_literals;

namespace Gloo::Internal::MicDetector {
void MicrophoneDetector::resume() { mgr_->startTracking(); }

void MicrophoneDetector::pause() {
  mgr_->stopTracking();
  {
    std::unique_lock<std::mutex> lk(m_);
    callbacks_.clear();
  }
}

int MicrophoneDetector::registerCallback(MicStatusCallback callback) {
  std::unique_lock<std::mutex> lk(m_);

  const int nextId = this->callbacks_.size();
  this->callbacks_[nextId] = callback;
  return nextId;
}

void MicrophoneDetector::unregisterCallback(int callbackId) {
  std::unique_lock<std::mutex> lk(m_);

  this->callbacks_.erase(callbackId);
}

void MicrophoneDetector::stateChanged(IDeviceManager::MicActivity state) const {
  const auto eachCb = [state](Napi::Env env, std::vector<napi_value> &args) {
    const auto eventName = Napi::String::New(env, "mic");
    const auto micData =
        Napi::Boolean::New(env, state == IDeviceManager::MicActivity::kOn);
    spdlog::debug("Firing mic event w data");
    args = {eventName, micData};
  };

  std::unique_lock<std::mutex> lk(m_);
  for (auto &[k, cb] : this->callbacks_) {
    cb->call(eachCb);
  }
}

}  // namespace Gloo::Internal::MicDetector
