#include <napi.h>
#include <spdlog/spdlog.h>

#include "mic_detector/MicDetector.h"
#include "napi-thread-safe-callback.h"
#include "screen_tracker/ScreenTracker.h"

using Gloo::Internal::MicDetector::MicrophoneDetector;
using Gloo::Internal::MicDetector::MicrophoneState;

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  spdlog::set_pattern("[%H:%M:%S %z] [Gloo-Native] [thread %t] %v");

  // For now we use verbose logging. Eventually we can losen this up.
  spdlog::set_level(spdlog::level::debug);

  spdlog::info("VERSION: 5.0.0");
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

  exports.Set(Napi::String::New(env, "enableWindowTracking"),
              Napi::Function::New(env, [&](const Napi::CallbackInfo &info) {
                Napi::Env env = info.Env();
                auto onChange = std::make_shared<ThreadSafeCallback>(
                    info[0].As<Napi::Function>());
                ScreenTracker::instance().registerCallback(onChange);
                return env.Undefined();
              }));
  exports.Set(Napi::String::New(env, "startScreensharing"),
              Napi::Function::New(env, [&](const Napi::CallbackInfo &info) {
                Napi::Env env = info.Env();
                auto windowHandle = info[0].As<Napi::Number>().Uint32Value();
                WindowTracker::instance().trackWindow(windowHandle);
                return env.Undefined();
              }));
  exports.Set(Napi::String::New(env, "stopScreensharing"),
              Napi::Function::New(env, [&](const Napi::CallbackInfo &info) {
                Napi::Env env = info.Env();
                WindowTracker::instance().stopTrackingWindow();
                return env.Undefined();
              }));
  return exports;
}

NODE_API_MODULE(mic_detector, Init);
