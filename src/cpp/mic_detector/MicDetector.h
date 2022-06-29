#pragma once

#include <napi.h>
#include <unordered_map>
#include <thread>
#include <memory>
#include <condition_variable>
#include <iostream>
#include "../napi-thread-safe-callback.h"

namespace Gloo::Internal::MicDetector
{
    enum class MicrophoneState : int
    {
        ON,
        OFF,
    };

    class MicrophoneDetector
    {
        typedef std::shared_ptr<ThreadSafeCallback> MicStatusCallback;

    public:
        // Make this object a singleton.
        MicrophoneDetector(const MicrophoneDetector&) = delete;
        MicrophoneDetector& operator=(const MicrophoneDetector&) = delete;
        MicrophoneDetector(MicrophoneDetector&&) = delete;
        MicrophoneDetector& operator=(MicrophoneDetector&&) = delete;
        static auto& instance()
        {
            static MicrophoneDetector _val;
            return _val;
        }

        // Starts an internal thread which monitors for microphone usage.
        void resume();
        // Pauses an internal thread which monitors for microphone usage.
        void pause();

        // Configures the function which is called whenever the function enables.
        int registerCallback(MicStatusCallback callback);
        void unregisterCallback(int callbackId);


    private:
        MicrophoneDetector() : thread_active_(false) {}
        ~MicrophoneDetector() {
            pause();
        }
        std::unordered_map<int, MicStatusCallback> callbacks_;
        bool thread_active_;

        // Used to fire callbacks.
        void stateChanged(MicrophoneState state) const;
        
        // Function run by thread.
        void Run();

        std::thread thread_;
        std::condition_variable cv;

        // Objects for thread safety.
        mutable std::mutex m_; // Used for safely accessing all members: callbacks_, thread_active_
    };
}
