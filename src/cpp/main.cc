#include "mic_detector/MicDetector.h"
#include "napi-thread-safe-callback.h"
#include <iostream>
#include <napi.h>

using Gloo::Internal::MicDetector::MicrophoneDetector;
using Gloo::Internal::MicDetector::MicrophoneState;

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  std::cout << "GLOO-NATIVE: VERSION: 3.1.1" << std::endl;
  exports.Set(Napi::String::New(env, "startMicrophoneDetection"),
              Napi::Function::New(env, [&](const Napi::CallbackInfo &info) {
                Napi::Env env = info.Env();
                auto emit = std::make_shared<ThreadSafeCallback>(
                    info[0].As<Napi::Function>());
                // We don't value the result here.
                (void)MicrophoneDetector::instance().registerCallback(emit);
                MicrophoneDetector::instance().resume();
                return env.Undefined();
              }));
  exports.Set(Napi::String::New(env, "stopMicrophoneDetection"),
              Napi::Function::New(env, [&](const Napi::CallbackInfo &info) {
                Napi::Env env = info.Env();
                MicrophoneDetector::instance().pause();
                return env.Undefined();
              }));
  return exports;
}

NODE_API_MODULE(mic_detector, Init);
