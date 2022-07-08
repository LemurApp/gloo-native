#include "MicDetector.h"
#include "DeviceInterface.h"
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

namespace Gloo::Internal::MicDetector
{
  void MicrophoneDetector::resume() {
    mgr_->start();
  }

  void MicrophoneDetector::pause() {
    mgr_->stop();
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

  void MicrophoneDetector::stateChanged(bool state) const {

    const auto eachCb = [state](Napi::Env env, std::vector<napi_value>& args) {
      const auto eventName = Napi::String::New(env, "mic");
      const auto micData = Napi::Boolean::New(env, state);
      args = {eventName,  micData};
    };

    std::unique_lock<std::mutex> lk(m_);
    for(auto& [k,cb] : this->callbacks_) {
      cb->call(eachCb);
    }
  }

}
