#include <ApplicationServices/ApplicationServices.h>

namespace Gloo::Internal::WindowManager
{
    namespace Darwin
    {
        class OSXWindowManager
        {
        public:
            OSXWindowManager();
            ~OSXWindowManager();

        private:
            AXUIElementRef frontmostApp_ = nullptr;
            AXObserverRef observer_ = nullptr;
        };
    }
}