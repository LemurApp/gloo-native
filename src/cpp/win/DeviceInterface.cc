#include "../mic_detector/DeviceInterface.h"
#include "MicrophoneDevice.h"
#include "helpers.h"
#include <mutex>

#include <windows.h>

#include <initguid.h>

#include <atlbase.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <uuids.h>

namespace Gloo::Internal::MicDetector {
HRESULT DeviceId(const CComPtr<IMMDevice> &device, std::wstring &deviceId) {
  deviceId = L"UKNOWN DEVICE ID";
  WCHAR *rawDeviceId = nullptr;
  RETURN_IF_FAILED(device->GetId(&rawDeviceId));
  deviceId = rawDeviceId;
  CoTaskMemFree(rawDeviceId);
  return S_OK;
}

class WindowsDeviceInterface final : public DeviceManager,
                                     public IMMNotificationClient {

public:
  WindowsDeviceInterface(DeviceManager::Callback cb,
                         CComPtr<IMMDeviceEnumerator> &deviceList)
      : DeviceManager(cb), pEnumerator(deviceList) {
    LOG_IF_FAILED(RefreshDeviceList());
    LOG_IF_FAILED(pEnumerator->RegisterEndpointNotificationCallback(this));
  }

  ~WindowsDeviceInterface() {
    LOG_IF_FAILED(pEnumerator->UnregisterEndpointNotificationCallback(this));
  }

  DEFAULT_ADDREF_RELEASE()
  QUERYINTERFACE_HELPER() {
    *object = nullptr;
    return E_NOINTERFACE;
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(
      /* [annotation][in] */
      _In_ LPCWSTR pwstrDeviceId,
      /* [annotation][in] */
      _In_ DWORD dwNewState) {
    std::wstring id(pwstrDeviceId);
    LOG_IF_FAILED(AddDevice(id));
    RefreshDeviceState();
    return S_OK;
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDeviceAdded(
      /* [annotation][in] */
      _In_ LPCWSTR pwstrDeviceId) {
    std::wcout << "Device Added: " << pwstrDeviceId << std::endl;
    return S_OK;
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDeviceRemoved(
      /* [annotation][in] */
      _In_ LPCWSTR pwstrDeviceId) {
    std::wcout << "Device Removed: " << pwstrDeviceId << std::endl;
    return S_OK;
  }

  /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
      /* [annotation][in] */
      _In_ EDataFlow flow,
      /* [annotation][in] */
      _In_ ERole role,
      /* [annotation][in] */
      _In_opt_ LPCWSTR pwstrDefaultDeviceId) {

    std::wcout << "Device Default changed: "
               << (pwstrDefaultDeviceId ? pwstrDefaultDeviceId : L"DEFAULT ID")
               << std::endl;
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
  bool IsActive() {
    std::unique_lock<std::mutex> lk_(m_);
    for (const auto &[_, d] : devices_) {
      if (d->IsActive()) {
        return true;
      }
    }
    return false;
  }

  void startImpl() {
    std::unique_lock<std::mutex> lk_(m_);
    for (auto &[_, d] : devices_) {
      LOG_IF_FAILED(d->StartListening());
    }
  }

  void stopImpl() {
    std::unique_lock<std::mutex> lk_(m_);
    for (auto &[_, d] : devices_) {
      LOG_IF_FAILED(d->StopListening());
    }
  }

  HRESULT RefreshDeviceList() {
    CComPtr<IMMDeviceCollection> collection;
    RETURN_IF_FAILED(pEnumerator->EnumAudioEndpoints(
        eCapture, DEVICE_STATEMASK_ALL, &collection))
    uint32_t count;
    RETURN_IF_FAILED(collection->GetCount(&count));
    std::wcout << "Adding Some Device: " << count << std::endl;
    for (uint32_t i = 0; i < count; ++i) {
      CComPtr<IMMDevice> device;
      RETURN_IF_FAILED(collection->Item(i, &device));
      std::wstring deviceId;
      RETURN_IF_FAILED(DeviceId(device, deviceId));
      LOG_IF_FAILED(AddDevice(deviceId, device));
    }

    return S_OK;
  }

  HRESULT AddDevice(const std::wstring &deviceId,
                    CComPtr<IMMDevice> device = nullptr) {
    std::wcout << "Adding Device: " << deviceId << std::endl;
    std::unique_lock<std::mutex> lk_(m_);
    if (devices_.count(deviceId)) {
      // Don't double track devices.
      return FWP_E_ALREADY_EXISTS;
    }

    if (!device) {
      RETURN_IF_FAILED(pEnumerator->GetDevice(deviceId.data(), &device));
    }

    DWORD deviceState;
    RETURN_IF_FAILED(device->GetState(&deviceState));
    if (deviceState != DEVICE_STATE_ACTIVE) {

      std::cout << "Device is not active: " << deviceState << std::endl;
      return E_NOT_VALID_STATE;
    }

    std::shared_ptr<MicrophoneDevice> spDevice;
    RETURN_IF_FAILED(MicrophoneDevice::Make(deviceId, device, spDevice, this));
    if (IsTracking()) {
      RETURN_IF_FAILED(spDevice->StartListening());
    }

    { devices_.insert_or_assign(deviceId, spDevice); }
    return S_OK;
  }

  HRESULT RemoveDevice(const std::wstring &deviceId) {
    std::wcout << "Removing Device: " << deviceId << std::endl;
    std::unique_lock<std::mutex> lk_(m_);
    auto res = devices_.erase(deviceId);
    if (res)
      return S_OK;
    return E_NOTFOUND;
  }

  CComPtr<IMMDeviceEnumerator> pEnumerator;

  std::mutex m_;
  std::unordered_map<std::wstring, std::shared_ptr<MicrophoneDevice>> devices_;
};

HRESULT MakeDeviceManagerInternal(DeviceManager::Callback cb,
                                  std::shared_ptr<DeviceManager> &output) {
  RETURN_IF_FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED |
                                               COINIT_DISABLE_OLE1DDE));
  CComPtr<IMMDeviceEnumerator> pEnumerator;
  RETURN_IF_FAILED(pEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                                NULL, CLSCTX_ALL));
  output.reset(new WindowsDeviceInterface(cb, pEnumerator));
  return S_OK;
}

std::shared_ptr<DeviceManager> MakeDeviceManager(DeviceManager::Callback cb) {
  std::shared_ptr<DeviceManager> output;
  LOG_IF_FAILED(MakeDeviceManagerInternal(cb, output));
  return output;
}

} // namespace Gloo::Internal::MicDetector
