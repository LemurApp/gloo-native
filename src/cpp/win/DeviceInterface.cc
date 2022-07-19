#include "../mic_detector/DeviceManager.h"
#include "MicrophoneDevice.h"
#include "helpers.h"
#include <mutex>
#include <unordered_set>
#include <vector>
#include <codecvt>

#include <windows.h>

#include <initguid.h>

#include <atlbase.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <uuids.h>

namespace Gloo::Internal::MicDetector {
namespace Windows {
HRESULT GetDeviceId(const CComPtr<IMMDevice> &device, AudioDeviceId &deviceId) {
  deviceId = "UKNOWN DEVICE ID";
  WCHAR *rawDeviceId = nullptr;
  RETURN_IF_FAILED(device->GetId(&rawDeviceId));
  deviceId = to_utf8(rawDeviceId);
  CoTaskMemFree(rawDeviceId);
  return S_OK;
}

class WindowsDeviceInterface final : public DeviceManager,
                                     public IMMNotificationClient {

public:
  WindowsDeviceInterface(IDeviceManager::OnMicChangeCallback cb0,
                         IDeviceManager::OnVolumeChangeCallback cb1,
                         CComPtr<IMMDeviceEnumerator> &deviceList)
      : DeviceManager(cb0, cb1), pEnumerator(deviceList) {}

  DEFAULT_ADDREF_RELEASE()
  QUERYINTERFACE_HELPER() {
    *object = nullptr;
    return E_NOINTERFACE;
  }

  void startDeviceCallbacks() {
    LOG_IF_FAILED(pEnumerator->RegisterEndpointNotificationCallback(this));
  }

  void stopDeviceCallbacks() {
    LOG_IF_FAILED(pEnumerator->UnregisterEndpointNotificationCallback(this));
  }

  void refreshDeviceList(bool maybeInitializeDevice) {
    LOG_IF_FAILED(winRefreshDeviceList(maybeInitializeDevice));
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(
      /* [annotation][in] */
      _In_ LPCWSTR pwstrDeviceId,
      /* [annotation][in] */
      _In_ DWORD dwNewState) {
    refreshDeviceList(true);
    return S_OK;
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDeviceAdded(
      /* [annotation][in] */
      _In_ LPCWSTR pwstrDeviceId) {
      spdlog::debug("Device Added: {}", to_utf8(pwstrDeviceId));
    return S_OK;
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDeviceRemoved(
      /* [annotation][in] */
      _In_ LPCWSTR pwstrDeviceId) {    if (spdlog::get_level() >= spdlog::level::debug) {
      spdlog::debug("Device Removed: {}", to_utf8(pwstrDeviceId));
    }
    return S_OK;
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
      /* [annotation][in] */
      _In_ EDataFlow flow,
      /* [annotation][in] */
      _In_ ERole role,
      /* [annotation][in] */
      _In_opt_ LPCWSTR pwstrDefaultDeviceId) {
    if (spdlog::get_level() >= spdlog::level::debug) {
      auto devId = pwstrDefaultDeviceId ? to_utf8(pwstrDefaultDeviceId) : "NONE";
      spdlog::debug("Device Default changed: {}", devId);
    }
    return S_OK;
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(
      /* [annotation][in] */
      _In_ LPCWSTR pwstrDeviceId,
      /* [annotation][in] */
      _In_ const PROPERTYKEY key) {
    return S_OK;
  }

private:

  HRESULT winRefreshDeviceList(bool maybeInitializeDevice) {
    // Get all input devices.
    std::unordered_map<AudioDeviceId, CComPtr<IMMDevice>> input_devices;

    CComPtr<IMMDeviceCollection> collection;
    RETURN_IF_FAILED(pEnumerator->EnumAudioEndpoints(
        eCapture, DEVICE_STATE_ACTIVE, &collection));
    uint32_t count;
    RETURN_IF_FAILED(collection->GetCount(&count));
    for (uint32_t i = 0; i < count; ++i) {
      CComPtr<IMMDevice> device;
      RETURN_IF_FAILED(collection->Item(i, &device));
      AudioDeviceId deviceId;
      RETURN_IF_FAILED(GetDeviceId(device, deviceId));
      input_devices[deviceId] = device;
    }

    std::vector<std::shared_ptr<IMicrophoneDevice>> devicesToStart;
    {
      std::unique_lock<std::mutex> lk_(_m);
      for (auto it = begin(_mics); it != end(_mics);) {
        const auto key = it->first;
        if (input_devices.count(key) == 0) {
          // Item is no longer present, remove it.
          it = _mics.erase(it);
        } else {
          // Already tracking this item, don't recreate it.
          input_devices.erase(key);
          ++it;
        }
      }
      bool withTrackingOn = isTracking();
      for (auto &[id, comDevice] : input_devices) {
        std::shared_ptr<IMicrophoneDevice> device;
        IF_SUCCEEDED(
            MicrophoneDevice::Make(id, comDevice, device, this)) {
          _mics[id] = device;
          if (maybeInitializeDevice && withTrackingOn) {
            devicesToStart.push_back(device);
          }
        }
      }
    }

    for (auto &dev : devicesToStart) {
      dev->startTracking();
    }

    return S_OK;
  }

  CComPtr<IMMDeviceEnumerator> pEnumerator;
};

HRESULT MakeDeviceManagerInternal(IDeviceManager::OnMicChangeCallback cb0,
                                  IDeviceManager::OnVolumeChangeCallback cb1,
                                  std::shared_ptr<DeviceManager> &output) {
  RETURN_IF_FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED |
                                               COINIT_DISABLE_OLE1DDE));
  CComPtr<IMMDeviceEnumerator> pEnumerator;
  RETURN_IF_FAILED(pEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                                NULL, CLSCTX_ALL));
  output.reset(new WindowsDeviceInterface(cb0, cb1, pEnumerator));
  return S_OK;
}
} // namespace Windows

std::shared_ptr<DeviceManager>
MakeDeviceManager(IDeviceManager::OnMicChangeCallback cb0,
                  IDeviceManager::OnVolumeChangeCallback cb1) {
  std::shared_ptr<DeviceManager> output;
  LOG_IF_FAILED(Windows::MakeDeviceManagerInternal(cb0, cb1, output));
  return output;
}

} // namespace Gloo::Internal::MicDetector
