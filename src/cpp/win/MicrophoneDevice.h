#pragma once

#include <atlbase.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <windows.h>

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

#include "../mic_detector/IDevice.h"
#include "helpers.h"

namespace Gloo::Internal::MicDetector {
namespace Windows {
class MicrophoneDevice final : public IAudioSessionNotification,
                               public IAudioSessionEvents,
                               public IMicrophoneDevice {

                                MicrophoneDevice(AudioDeviceId deviceId, IDeviceManager *manager,
                   CComPtr<IAudioSessionManager2> sessionManager)
      : IMicrophoneDevice(deviceId, manager), _sessionManager(sessionManager) {}
 public:
  static HRESULT MicrophoneDevice::Make(const AudioDeviceId &deviceId,
                               CComPtr<IMMDevice> &device,
                               std::shared_ptr<IMicrophoneDevice> &output,
                               IDeviceManager *manager);

  DEFAULT_ADDREF_RELEASE()
  QUERYINTERFACE_HELPER() {
    *object = nullptr;
    return E_NOINTERFACE;
  }

  ~MicrophoneDevice() { stopTracking(); }

  // ---
  // IAudioSessionNotification
  // ---
  HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl *session);

  // ---
  // IAudioSessionEvents
  // ---
  HRESULT STDMETHODCALLTYPE OnStateChanged(
      /* [annotation][in] */
      _In_ AudioSessionState state) {
    winUpdateState(state, false);
    return S_OK;
  }

  // The rest of the callbacks are currently not used.
  HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(_In_ LPCWSTR NewDisplayName,
                                                 LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnIconPathChanged(_In_ LPCWSTR NewIconPath,
                                              LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(_In_ float NewVolume,
                                                  _In_ BOOL NewMute,
                                                  LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE
  OnChannelVolumeChanged(_In_ DWORD ChannelCount,
                         _In_reads_(ChannelCount) float NewChannelVolumeArray[],
                         _In_ DWORD ChangedChannel, LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE
  OnGroupingParamChanged(_In_ LPCGUID NewGroupingParam, LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE
  OnSessionDisconnected(_In_ AudioSessionDisconnectReason DisconnectReason) {
    return S_OK;
  }

private:
  bool getStateFromDevice() const {
    auto state = _micActivity.load();
    spdlog::debug("Device state:{}: {}", _deviceId, state);
    return state > 0;
  }

  void startTrackingDeviceImpl() {
    LOG_IF_FAILED(winStartTrackingDeviceImpl());
  }
  void stopTrackingDeviceImpl() {
    LOG_IF_FAILED(winStopTrackingDeviceImpl());
    _micActivity.store(0);
  }
  HRESULT winStartTrackingDeviceImpl();
  HRESULT winStopTrackingDeviceImpl();
  void winUpdateState(AudioSessionState state, bool initialCall);


 private:
  std::atomic<int> _micActivity = 0;
  
  std::mutex _m;
  // Manages the lifetime of the device.
  const CComPtr<IAudioSessionManager2> _sessionManager;
  // Manages each instance of the device being used.
  std::vector<CComPtr<IAudioSessionControl2>> _sessionControllers;
};
}  // namespace Windows
}  // namespace Gloo::Internal::MicDetector
