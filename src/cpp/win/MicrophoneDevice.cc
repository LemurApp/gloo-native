#include "MicrophoneDevice.h"
#include <memory>
#include <vector>

#include <windows.h>

#include <atlbase.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <mmdeviceapi.h>

namespace Gloo::Internal::MicDetector {
HRESULT MicrophoneDevice::StartListening() {
  if (tracking_.load())
    return S_OK;

  CComPtr<IAudioSessionEnumerator> sessionList;
  RETURN_IF_FAILED(manager_->GetSessionEnumerator(&sessionList));
  int count;
  RETURN_IF_FAILED(sessionList->GetCount(&count));
  for (int i = 0; i < count; ++i) {
    CComPtr<IAudioSessionControl> controller;
    RETURN_IF_FAILED(sessionList->GetSession(i, &controller));
    RETURN_IF_FAILED(OnSessionCreated(controller));
  }

  std::cout << "Registering for RegisterSessionNotification" << std::endl;
  RETURN_IF_FAILED(manager_->RegisterSessionNotification(this));
  tracking_.store(true);
  return S_OK;
}

HRESULT MicrophoneDevice::StopListening() {
  if (!tracking_.load())
    return S_OK;

  RETURN_IF_FAILED(manager_->UnregisterSessionNotification(this));
  for (auto &controller : sessionControllers_) {
    RETURN_IF_FAILED(controller->UnregisterAudioSessionNotification(this));
  }
  sessionControllers_.clear();
  tracking_.store(false);
  return S_OK;
}

// Takes ownership of device.
HRESULT MicrophoneDevice::Make(const std::wstring &deviceId,
                               CComPtr<IMMDevice> &device,
                               std::shared_ptr<MicrophoneDevice> &output,
                               DeviceManager *owner) {
  CComPtr<IAudioSessionManager2> manager;
  RETURN_IF_FAILED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                                    nullptr, (void **)(&manager)));
  output.reset(new MicrophoneDevice(owner, deviceId, manager));
  return S_OK;
}

HRESULT MicrophoneDevice::OnSessionCreated(IAudioSessionControl *session) {
  CComPtr<IAudioSessionControl2> audioSessionControl;
  if (SUCCEEDED(session->QueryInterface(&audioSessionControl))) {
    if (audioSessionControl->IsSystemSoundsSession() == S_FALSE) {
      CComHeapPtr<WCHAR> name;
      LOG_IF_FAILED(audioSessionControl->GetDisplayName(&name));

      std::wcout << "New session:(" << deviceId_ << ") "
                 << sessionControllers_.size() << " (" << name.m_pData << ")"
                 << std::endl;
      AudioSessionState state;
      RETURN_IF_FAILED(audioSessionControl->GetState(&state));
      RETURN_IF_FAILED(OnStateChanged(state, 0));
      RETURN_IF_FAILED(
          audioSessionControl->RegisterAudioSessionNotification(this));
      sessionControllers_.push_back(audioSessionControl);
    }
  }
  return S_OK;
}

HRESULT MicrophoneDevice::OnStateChanged(
    /* [annotation][in] */
    _In_ AudioSessionState state, _In_ int false_value) {
  switch (state) {
  case AudioSessionStateExpired:
    std::wcout << "Session Expired:(" << deviceId_ << ")" << std::endl;
    break;
  case AudioSessionStateInactive:
    UpdateState(false, false_value);
    break;
  case AudioSessionStateActive:
    UpdateState(true, false_value);
    break;
  }
  return S_OK;
}

void MicrophoneDevice::UpdateState(bool state, int false_value) {
  const int prev =
      inUse_.fetch_add(state ? 1 : false_value, std::memory_order_relaxed);
  std::wcout << "Device::(" << deviceId_ << ") " << (state ? "ON" : "OFF")
             << " " << prev << " --> " << (prev + (state ? 1 : false_value))
             << std::endl;
  if (false_value != 0) {
    owner_->RefreshDeviceState();
  }
}
} // namespace Gloo::Internal::MicDetector