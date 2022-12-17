#ifdef __APPLE__

#include <algorithm>
#include <memory>
#include <unordered_set>

#include "OSXHelpers.h"
#include <iostream>
#include "OSXWindowManager.h"

#define AXCHECK(f)                                                                                                          \
  {                                                                                                                         \
    AXError err##__LINE__ = f;                                                                                              \
    if (err##__LINE__ != kAXErrorSuccess)                                                                                   \
    {                                                                                                                       \
      std::cout << #f << " failed: " << __FILE__ << ":" << __LINE__ << ": " << axErrorToString(err##__LINE__) << std::endl; \
      throw std::invalid_argument(#f);                                                                                      \
    }                                                                                                                       \
  }

namespace Gloo::Internal::WindowManager
{
  namespace Darwin
  {
    const char *axErrorToString(AXError error)
    {
      switch (error)
      {
      case kAXErrorSuccess:
        return "Success";
      case kAXErrorFailure:
        return "Failure";
      case kAXErrorIllegalArgument:
        return "Illegal argument";
      case kAXErrorInvalidUIElement:
        return "Invalid UI element";
      case kAXErrorInvalidUIElementObserver:
        return "Invalid UI element observer";
      case kAXErrorCannotComplete:
        return "Cannot complete";
      case kAXErrorAttributeUnsupported:
        return "Attribute unsupported";
      case kAXErrorActionUnsupported:
        return "Action unsupported";
      case kAXErrorNotificationUnsupported:
        return "Notification unsupported";
      case kAXErrorNotImplemented:
        return "Not implemented";
      default:
        return "Unknown error";
      }
    }

    // Callback function that is called when the active window changes
    void activeWindowChanged(AXObserverRef observer, AXUIElementRef element, CFStringRef notification, void *data)
    {
      std::cout << "Active window changed " << std::endl;
      // Get the new active window
      AXUIElementRef activeWindow;
      AXCHECK(AXUIElementCopyAttributeValue(element, kAXFocusedWindowAttribute, (CFTypeRef *)&activeWindow));

      // Get the name of the active window
      // CFStringRef windowName;
      // error = AXUIElementCopyAttributeValue(activeWindow, kAXTitleAttribute, (CFTypeRef *)&windowName);
      // if (error != kAXErrorSuccess) {
      //   // Failed to get the window name
      //   CFRelease(activeWindow);
      //   return;
      // }

      // Get the position of the active window
      // CGPoint windowPosition;
      // error = AXUIElementCopyAttributeValue(activeWindow, kAXPositionAttribute, (CFTypeRef *)&windowPosition);
      // if (error != kAXErrorSuccess) {
      //   // Failed to get the window position
      //  // CFRelease(windowName);
      //   CFRelease(activeWindow);
      //   return;
      // }

      // Print the name and position of the active window
      std::cout << "Active window changed "
                // << CFStringGetCStringPtr(windowName, kCFStringEncodingUTF8) << " (" << windowPosition.x << ", " << windowPosition.y << ")"
                << std::endl;

      // Clean up
      // CFRelease(windowName);
      CFRelease(activeWindow);
    }

    pid_t GetPid()
    {
      if (AXIsProcessTrustedWithOptions(NULL))
      {
        AXUIElementRef activeWindow = AXUIElementCreateSystemWide();

        // AXLibWindowRef activeWindow = AXLibGetActiveWindow();
        std::cout << "Trust process: " << activeWindow << std::endl;
        pid_t pid;
        AXCHECK(AXUIElementGetPid(activeWindow, &pid));
        return pid;
      }
      return getpid();
    }

    OSXWindowManager::OSXWindowManager()
    {
      pid_t pid = GetPid();
      AXCHECK(AXObserverCreate(pid, activeWindowChanged, &observer_));
      frontmostApp_ = AXUIElementCreateApplication(pid);
      std::cout << "FrontApp: " << frontmostApp_ << std::endl;
      AXCHECK(AXObserverAddNotification(observer_, frontmostApp_, kAXFocusedUIElementChangedNotification, nullptr));
    }
    OSXWindowManager::~OSXWindowManager()
    {
      CFRelease(frontmostApp_);
      CFRelease(observer_);
    }
  } // namespace Darwin
} // namespace Gloo::Internal::MicDetector
#endif
