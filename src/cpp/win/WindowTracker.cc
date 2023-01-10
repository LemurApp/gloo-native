#include "../screen_tracker/WindowTracker.h"

#include <windows.h>

void WindowTracker::UpdateWindows() {
  while (runWorker_.load()) {
    auto target = GetTargetWindow();
    if (target != UINT32_MAX) {
      RECT rect = {NULL};
      if (GetWindowRect((HWND)target, &rect)) {
        WindowData data;
        data.id = target;
        WindowRect& w = data.position;
        w.x = rect.left;
        w.y = rect.top;
        w.width = rect.right - rect.left;
        w.height = rect.bottom - rect.top;
        OnTargetWindow(&data);
      }
    } else {
      OnTargetWindow(nullptr);
    }

    // Rate limit to run every 2 second.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
