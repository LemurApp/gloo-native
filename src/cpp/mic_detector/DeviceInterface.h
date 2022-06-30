#pragma once

#include <string>
#include <atomic>
namespace Gloo::Internal::MicDetector
{

    class DeviceManager
    {

    public:
        typedef std::function<void(bool)> Callback;
        
        DeviceManager(Callback cb) : callback_(cb) {}
        
        virtual ~DeviceManager() { stop(); }

        void start()
        {
            const bool prev = running_.exchange(true);
            if (prev) return;

            micIsActive_.store(0);
            RefreshDeviceState();
            startImpl();
        }
        void stop()
        {
            const bool prev = running_.exchange(false);
            if (prev) {
                stopImpl();
            }
        }

        void RefreshDeviceState() {
            SetActive(IsActive());
        }

    protected:
        void SetActive(bool value)
        {
            const auto next = value ? 2 : 1;
            const auto prev = micIsActive_.exchange(next);
            if (next != prev)
            {
                callback_(next == 2);
            }
        }

        virtual bool IsActive() = 0;
        virtual void startImpl() = 0;
        virtual void stopImpl() = 0;

    private:
        // This int is used like an enum
        // 0: UNKNOWN
        // 1: MIC OFF
        // 2: MIC ON
        Callback callback_;
        
        std::atomic<bool> running_{0};
        std::atomic<int> micIsActive_;
    };

    std::shared_ptr<DeviceManager> MakeDeviceManager(DeviceManager::Callback cb);

};
