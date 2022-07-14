#pragma once

#include <atlbase.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <windows.h>

#include <memory>
#include <vector>

#include "../mic_detector/DeviceInterface.h"
#include "helpers.h"

namespace Gloo::Internal::MicDetector {
namespace Windows {
class MicrophoneDevice final : public IAudioSessionNotification,
                               public IAudioSessionEvents,
                               public IMicrophoneDevice {
  MicrophoneDevice(AudioDeviceId deviceId, IDeviceManager *manager,
                   CComPtr<IAudioSessionManager2> session_manager)
      : IMicrophoneDevice(deviceId, manager), manager_(session_manager) {}

 public:
  DEFAULT_ADDREF_RELEASE()
  QUERYINTERFACE_HELPER() {
    *object = nullptr;
    return E_NOINTERFACE;
  }

  ~MicrophoneDevice() { stopTracking(); }

  bool getStateFromDevice() const;

  void startTrackingDeviceImpl();
  void stopTrackingDeviceImpl();

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
    this->refreshState(state == AudioSessionState::kConnected);
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
  std::atomic<AudioSessionState> state_;

  const CComPtr<IAudioSessionManager2> manager_;
};
}  // namespace Windows
}  // namespace Gloo::Internal::MicDetector
