#include "WindowTracker.h"

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>

#include "OSXHelpers.h"

bool starts_with(const std::string &s, const std::string &prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool ParseDictionaryBool(CFDictionaryRef data, CFStringRef key) {
  if (CFDictionaryContainsKey(data, key)) {
    CFTypeRef val = CFDictionaryGetValue(data, key);
    if (CFGetTypeID(val) == CFBooleanGetTypeID()) {
      return CFBooleanGetValue((CFBooleanRef)val);
    }
  }
  return false;
}

uint32_t ParseDictionaryU32(CFDictionaryRef data, CFStringRef key) {
  if (CFDictionaryContainsKey(data, key)) {
    CFTypeRef val = CFDictionaryGetValue(data, key);
    if (CFGetTypeID(val) == CFNumberGetTypeID()) {
      uint32_t parsed;
      CFNumberGetValue((CFNumberRef)val, kCFNumberIntType, &parsed);
      return parsed;
    }
  }
  throw std::invalid_argument("Unable to convert u32");
}

std::optional<std::string> ParseDictionaryString(CFDictionaryRef data,
                                                 CFStringRef key) {
  if (CFDictionaryContainsKey(data, key)) {
    CFTypeRef val = CFDictionaryGetValue(data, key);
    if (CFGetTypeID(val) == CFStringGetTypeID()) {
      return Gloo::Internal::MicDetector::Darwin::OSX_FromCfString(
          (CFStringRef)val);
    }
  }
  return std::nullopt;
}

void GetWindowData(CFDictionaryRef window, WindowData &parsed) {
  parsed.windowName = ParseDictionaryString(window, kCGWindowName);
  parsed.parentName = ParseDictionaryString(window, kCGWindowOwnerName);
  parsed.id = ParseDictionaryU32(window, kCGWindowNumber);
  parsed.parentPid = ParseDictionaryU32(window, kCGWindowOwnerPID);
  CGRect rect;
  if (CGRectMakeWithDictionaryRepresentation(
          (CFDictionaryRef)CFDictionaryGetValue(window, kCGWindowBounds),
          &rect)) {
    parsed.position.x = rect.origin.x;
    parsed.position.y = rect.origin.y;
    parsed.position.width = rect.size.width;
    parsed.position.height = rect.size.height;
  }
}

std::vector<WindowData> GetAllWindows() {
  // Fetch all windows which are on screen.
  CFArrayRef windowList = CGWindowListCopyWindowInfo(
      kCGWindowListOptionOnScreenOnly, kCGNullWindowID);

  // Find the window with the matching windowId
  const CFIndex numWindows = CFArrayGetCount(windowList);
  std::vector<WindowData> windows(numWindows);
  for (CFIndex i = 0; i < numWindows; i++) {
    CFTypeRef window_unsafe_ = CFArrayGetValueAtIndex(windowList, i);
    if (CFGetTypeID(window_unsafe_) != CFDictionaryGetTypeID()) {
      continue;
    }
    CFDictionaryRef window = (CFDictionaryRef)window_unsafe_;
    GetWindowData(window, windows[i]);
  }
  return windows;
}

void WindowTracker::UpdateWindows() {
  while (runWorker_.load()) {
    const auto data = GetAllWindows();
    bool hasStatusIndicator = false;
    for (const WindowData &w : data) {
      if (w.parentName.has_value() &&
          starts_with(*w.parentName, "Window Server")) {
        if (w.windowName.has_value() &&
            starts_with(*w.windowName, "StatusIndicator")) {
          hasStatusIndicator = true;
          break;
        }
      }
    }
    OnStatusIndicator(hasStatusIndicator);

    auto target = GetTargetWindow();
    if (target != UINT32_MAX) {
      bool found = false;
      for (const WindowData &w : data) {
        if (w.id == target) {
          // Emit data about the targets position.
          OnTargetWindow(&w);
          found = true;
          break;
        }
      }
      if (!found) {
        OnTargetWindow(nullptr);
      }
    }

    // Rate limit to run every 2 second.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
