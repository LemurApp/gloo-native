#pragma once

#include "../mic_detector/DeviceInterface.h"
#include "helpers.h"
#include <memory>
#include <vector>

#include <windows.h>

#include <atlbase.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <mmdeviceapi.h>

namespace Gloo::Internal::MicDetector {
class MicrophoneDevice final : public IAudioSessionNotification,
                               public IAudioSessionEvents {
  MicrophoneDevice(DeviceManager *owner, const std::wstring &deviceId,
                   CComPtr<IAudioSessionManager2> manager)
      : deviceId_(deviceId), manager_(manager), owner_(owner) {}

public:
  DEFAULT_ADDREF_RELEASE()
  QUERYINTERFACE_HELPER() {
    *object = nullptr;
    return E_NOINTERFACE;
  }

  ~MicrophoneDevice() { LOG_IF_FAILED(StopListening()); }

  std::wstring Id() const { return deviceId_; }

  HRESULT StartListening();
  HRESULT StopListening();

  bool IsActive() const { return inUse_.load(); }

  // Takes ownership of device.
  static HRESULT Make(const std::wstring &deviceId, CComPtr<IMMDevice> &device,
                      std::shared_ptr<MicrophoneDevice> &output,
                      DeviceManager *owner);

  // ---
  // IAudioSessionNotification
  // ---
  HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl *session);

  // ---
  // IAudioSessionEvents
  // ---
  HRESULT STDMETHODCALLTYPE OnStateChanged(
      /* [annotation][in] */
      _In_ AudioSessionState state, _In_ int false_value);
  HRESULT STDMETHODCALLTYPE OnStateChanged(
      /* [annotation][in] */
      _In_ AudioSessionState state) {
    return OnStateChanged(state, -1);
  }

  // The rest of the callbacks are currently not used.
  HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(
      /* [annotation][string][in] */
      _In_ LPCWSTR NewDisplayName,
      /* [in] */ LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnIconPathChanged(
      /* [annotation][string][in] */
      _In_ LPCWSTR NewIconPath,
      /* [in] */ LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(
      /* [annotation][in] */
      _In_ float NewVolume,
      /* [annotation][in] */
      _In_ BOOL NewMute,
      /* [in] */ LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(
      /* [annotation][in] */
      _In_ DWORD ChannelCount,
      /* [annotation][size_is][in] */
      _In_reads_(ChannelCount) float NewChannelVolumeArray[],
      /* [annotation][in] */
      _In_ DWORD ChangedChannel,
      /* [in] */ LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(
      /* [annotation][in] */
      _In_ LPCGUID NewGroupingParam,
      /* [in] */ LPCGUID EventContext) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnSessionDisconnected(
      /* [annotation][in] */
      _In_ AudioSessionDisconnectReason DisconnectReason) {
    return S_OK;
  }

private:
  void UpdateState(bool state, int false_value = -1);

  const std::wstring deviceId_;

  std::atomic<bool> tracking_ = false;
  std::atomic<int> inUse_;
  const CComPtr<IAudioSessionManager2> manager_;

  std::vector<CComPtr<IAudioSessionControl2>> sessionControllers_;
  DeviceManager *owner_;
};
} // namespace Gloo::Internal::MicDetector
