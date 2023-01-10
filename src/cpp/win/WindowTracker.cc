#include "../screen_tracker/WindowTracker.h"

#include <windef.h>

void WindowTracker::UpdateWindows() {
  while (runWorker_.load()) {
    auto target = GetTargetWindow();
    if (target != UINT32_MAX) {
      RECT rect = {NULL};
      if (GetWindowRect(target, &rect)) {
        WindowRect w;
        w.x = rect.left;
        w.y = rect.top;
        w.width = rect.right - rect.left;
        w.height = rect.bottom - rect.top;
        OnTargetWindow(&w);
      }
    } else {
      OnTargetWindow(nullptr);
    }

    // Rate limit to run every 2 second.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
