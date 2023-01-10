#include "ScreenTracker.h"

#include <iostream>

void ScreenTracker::onChange(bool visibility, const WindowRect *rect) {
  WindowRect r;
  if (visibility) {
    memcpy(&r, rect, sizeof(WindowRect));
  }
  const auto eachCb = [visibility, r](Napi::Env env,
                                      std::vector<napi_value> &args) {
    if (visibility) {
      const auto eventName = Napi::String::New(env, "move");
      auto position = Napi::Object::New(env);
      position.Set("x", r.x);
      position.Set("y", r.y);
      position.Set("width", r.width);
      position.Set("height", r.height);
      args = {eventName, position};
    } else {
      const auto eventName = Napi::String::New(env, "hide");
      args = {eventName};
    }
  };

  std::unique_lock<std::mutex> lk(m_);
  for (auto &[k, cb] : this->callbacks_) {
    cb->call(eachCb);
  }
}
