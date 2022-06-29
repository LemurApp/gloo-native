#include "MicDetector.h"
#include "DeviceInterface.h"
#include <thread>
#include <iostream>
#include <chrono>
using namespace std::chrono_literals;

namespace Gloo::Internal::MicDetector
{
  MicrophoneState GetCurrentMicState() {
    auto devices = GetInputDevices();
    auto states = GetDeviceActiveState(devices);
      
    bool isInUse = false;
    for (auto& s: states) {
      isInUse |= s.isDeviceInUse;
    }
    return isInUse ? MicrophoneState::ON : MicrophoneState::OFF;
  }


  void MicrophoneDetector::resume() {
    if (thread_.joinable()) {
      // Thread is already running.
      return;
    }
    
    {
      std::unique_lock<std::mutex> lk(m_);
      thread_active_ = true;
    }

    thread_ = std::thread(&MicrophoneDetector::Run, this);
  }

  void MicrophoneDetector::pause() {
    if (!thread_.joinable()) {
      return;
    }

    {
      std::unique_lock<std::mutex> lk(m_);
      thread_active_ = false;
    }
    // Wait for the thread to close.
    thread_.join();
    callbacks_.clear();
  }

  void MicrophoneDetector::Run() {
    MicrophoneState lastInUseStatus = MicrophoneState::OFF;

    while (true) {
      {
        std::unique_lock<std::mutex> lk(m_);
        if (this->cv.wait_for(lk, 1s, [&]{return !this->thread_active_;})) {
          return;
        }
      }

      const auto state = GetCurrentMicState();
      if (lastInUseStatus != state) {
        // std::cout << "Mic Status CPP: " << (lastInUseStatus == MicrophoneState::ON ? "ON" : "OFF") << "->" << (state == MicrophoneState::ON ? "ON" : "OFF") << std::endl;
        this->stateChanged(state);
        lastInUseStatus = state;
      }
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

  void MicrophoneDetector::stateChanged(MicrophoneState state) const {
    static int counter = 0;
    std::unique_lock<std::mutex> lk(m_);

    const auto isOn = state == MicrophoneState::ON;
    const auto eachCb = [isOn](Napi::Env env, std::vector<napi_value>& args) {
      const auto eventName = Napi::String::New(env, "mic");
      const auto micData = Napi::Boolean::New(env, isOn);
      args = {eventName,  micData};
    };

    for(auto& [k,cb] : this->callbacks_) {
      cb->call(eachCb);
    }
    counter++;
  }

}
