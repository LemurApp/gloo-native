#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include <stdexcept>
#include <system_error>
#include <vector>


namespace Gloo::Internal::MicDetector::Darwin {
    uint32_t SafeAudioObjectGetPropertySize(AudioObjectID deviceId, const AudioObjectPropertyAddress& props) {
        if (!AudioObjectHasProperty(deviceId, &props)) {
            throw std::invalid_argument("No such property on device");
        }

        uint32_t property_size;
        const auto err = AudioObjectGetPropertyDataSize(deviceId, &props, 0, nullptr, &property_size);
        if (err) {
            throw std::system_error(EDOM, std::generic_category(), "hello world");
        }
        return property_size;
    }

    template<typename T>
    class DataBuffer {
        public:
        DataBuffer(uint32_t size): buffer_(size), count_(size / sizeof(T)) {}

        void forEach( std::function<void(T)> cb) {
            const T* content = cast_data();
            for (int i = 0; i < count_; ++i) {
                cb(content[i]);
            }
        }
        
        const T* cast_data() const { return reinterpret_cast<const T*>(data()); }
        T* cast_data() { return reinterpret_cast<T*>(data()); }
        
        const void* data() const { return buffer_.data(); }
        void* data() { return buffer_.data(); }
        private:
        std::vector<uint8_t> buffer_;
        const uint32_t count_;
    };

    template<typename T>
    DataBuffer<T> SafeAudioObjectGetPropertyValue(AudioObjectID deviceId, const AudioObjectPropertyAddress& props, uint32_t size = 0) {
        if (size == 0) {
            size = SafeAudioObjectGetPropertySize(deviceId, props);
        }

        DataBuffer<T> buffer(size);
        const auto err = AudioObjectGetPropertyData(
                deviceId, &props, 0, nullptr, &size, buffer.data());
        if (err) {
            throw std::system_error(EDOM, std::generic_category(), "hello world");
        }
        return buffer;
    }
}
